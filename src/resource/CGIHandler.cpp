#include "CGIHandler.hpp"
#include "HTTP/core/HttpResponse.hpp"
#include "utils/Logger.hpp"

using HTTP::Request;
using HTTP::Method;
using HTTP::StatusCode;
using HTTP::methodToString;
using HTTP::statusToString;
using HTTP::getMimeType;

CGIHandler::CGIHandler(const std::string &root) : _root_directory(root) {
  Logger::infof("CGIHandler initialized with root directory: %s", root.c_str());
  registerHandler(".php", "/usr/bin/php");
  registerHandler(".py", "/usr/bin/python3");
  registerHandler(".pl", "/usr/bin/perl");
}

CGIHandler::~CGIHandler() {}

bool CGIHandler::canHandle(const std::string &filePath) const {
  size_t dot_pos = filePath.find_last_of('.');
  if (dot_pos == std::string::npos) {
    return false;
  }
  std::string extension = filePath.substr(dot_pos);
  return _cgi_handlers.find(extension) != _cgi_handlers.end();
}

std::string CGIHandler::executeCGI(const std::string &uri, const Request &request) {
  std::string filePath = _root_directory + uri;
  size_t dot_pos = filePath.find_last_of('.');

  if (dot_pos == std::string::npos)
    return "";
  std::string extension = filePath.substr(dot_pos);
  auto handlerIt = _cgi_handlers.find(extension);
  if (handlerIt == _cgi_handlers.end())
    return "";

  return executeScript(filePath, handlerIt->second, request);
}

std::string CGIHandler::executeScript(const std::string &script_path,
                                      const std::string &handler_path,
                                      const Request &request) {

  int pipefd[2];
  if (pipe(pipefd) == -1)
    return HttpResponse::internalError(std::string_view("Failed to create pipe"));

  int input_pipe[2] = {-1, -1};
  if (request.requestLine.method == Method::POST && !request.body.empty()) {
    if (pipe(input_pipe) == -1) {
      close(pipefd[0]);
      close(pipefd[1]);
      return HttpResponse::internalError(std::string_view("Failed to create input pipe"));
    }
  }

  pid_t pid = fork();
  if (pid < 0) {
    close(pipefd[0]);
    close(pipefd[1]);

    if (input_pipe[0] != -1) {
      close(input_pipe[0]);
      close(input_pipe[1]);
    }
    return HttpResponse::internalError(std::string_view("Fork failed"));
  }

  if (pid == 0) {
    dup2(pipefd[1], STDOUT_FILENO);
    close(pipefd[0]);
    close(pipefd[1]);

    if (input_pipe[0] != -1) {
      dup2(input_pipe[0], STDIN_FILENO);
      close(input_pipe[0]);
      close(input_pipe[1]);
    }

    std::vector<char *> args;
    args.push_back(const_cast<char *>(handler_path.c_str()));
    args.push_back(const_cast<char *>(script_path.c_str()));
    args.push_back(nullptr);

    std::vector<std::string> env_vars;
    env_vars.emplace_back("GATEWAY_INTERFACE=CGI/1.1");
    env_vars.emplace_back("REQUEST_METHOD=" + methodToString(request.requestLine.method));
    env_vars.emplace_back("SCRIPT_NAME=" + script_path);
    env_vars.emplace_back("SERVER_PROTOCOL=" + request.requestLine.version);
    env_vars.emplace_back("SERVER_SOFTWARE=webserv/1.0");

    size_t pos = request.requestLine.uri.find('?');
    std::string queryStr = (pos != std::string::npos) ? request.requestLine.uri.substr(pos + 1) : "";
    env_vars.emplace_back("QUERY_STRING=" + queryStr);

    if (auto it = request.headers.find("CONTENT_TYPE"); it != request.headers.end())
      env_vars.emplace_back("CONTENT_TYPE=" + it->second);
    if (auto it = request.headers.find("CONTENT_LENGTH"); it != request.headers.end())
      env_vars.emplace_back("CONTENT_LENGTH=" + it->second);

    std::vector<char *> envp;
    for (auto& env : env_vars)
      envp.push_back(const_cast<char *>(env.c_str()));
    envp.push_back(nullptr);

    execve(handler_path.c_str(), args.data(), envp.data());
    _exit(EXIT_FAILURE);
  }

  if (input_pipe[0] != -1) {
    close(input_pipe[0]);
    write(input_pipe[1], request.body.c_str(), request.body.size());
    close(input_pipe[1]);
  }
  close(pipefd[1]);
  std::ostringstream output;
  char buffer[4096];
  ssize_t bytesRead;
  while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
    output.write(buffer, bytesRead);
  }
  close(pipefd[0]);
  int status;
  waitpid(pid, &status, 0);

  if (WIFEXITED(status) && WEXITSTATUS(status) != 0)
    return HttpResponse::internalError("CGI script execution failed with status: " + std::to_string(WEXITSTATUS(status)));

  return parseCGIOutput(output.str());
}

std::string CGIHandler::parseCGIOutput(const std::string& output){
  size_t header_end = output.find("\r\n\r\n");
  size_t header_separator_len = 4;

  if (header_end == std::string::npos) {
    header_end = output.find("\n\n");
    header_separator_len = 2;
  }

  if (header_end == std::string::npos)
    return HttpResponse::internalError("CGI output doesn't have valid headers");

  std::string header_section = output.substr(0, header_end);
  std::string body = output.substr(header_end + header_separator_len);

  std::map<std::string, std::string> headers;
  std::istringstream header_stream(header_section);
  std::string line;
  StatusCode status_code = StatusCode::OK;

  while (std::getline(header_stream, line)) {
    if (!line.empty() && line.back() == '\r')
      line.pop_back();

    size_t colon_pos = line.find(':');
    if (colon_pos != std::string::npos) {
      std::string key = line.substr(0, colon_pos);
      std::string value = line.substr(colon_pos + 1);

      value.erase(0, value.find_first_not_of(" \t"));
      if (key == "Status") {
        try {
          int status_num = std::stoi(value.substr(0, 3));
          status_code = static_cast<StatusCode>(status_num);
        } catch (...) {
          return HttpResponse::internalError("Invalid status code in CGI output");
        }
      }
      else
        headers[key] = value;
    }
  }
  if (headers.find("Content-Type") == headers.end())
    headers["Content-Type"] = getMimeType(body);

  std::string response = HttpResponse::buildResponse(
    static_cast<int>(status_code),
    std::string(statusToString(status_code)),
    std::string_view(body),
    std::string_view(headers["Content-Type"]));

  for (const auto& header : headers) {
    if (header.first != "Content-Type") {
      size_t headerPos = response.find("\r\n\r\n");
      if (headerPos != std::string::npos) {
        response.insert(headerPos, "\r\n" + header.first + ": " + header.second);
      }
    }
  }

  return response;
}

void CGIHandler::registerHandler(const std::string &extension, const std::string &handlerPath) {
  _cgi_handlers[extension] = handlerPath;
}

void CGIHandler::setRootDirectory(const std::string &root) {
  _root_directory = root;
}
