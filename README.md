# webserv

> Advanced HTTP/1.1 server in C++17 — 42 School Project

![C++17](https://img.shields.io/badge/C++-17-blue?style=flat-square&logo=c%2B%2B&logoColor=white)
![Linux](https://img.shields.io/badge/OS-Linux-purple?style=flat-square&logo=linux)
![Non-blocking I/O](https://img.shields.io/badge/Non--blocking%20I/O-Enabled-darkblue?style=flat-square)
![HTTP](https://img.shields.io/badge/HTTP-1.1-informational?style=flat-square&logo=apache)
[![42 Project](https://img.shields.io/badge/42%20Project-blueviolet?style=flat-square)](https://42.fr)

---

## 🚀 Overview

**webserv** is a non-blocking HTTP/1.1 web server written in modern C++ (following C++98/C++17 guidelines for the 42 school project).  
It supports multiple simultaneous connections, serves static sites, handles CGI for dynamic content, file uploads, and full configuration flexibility inspired by NGINX.

---

## 📦 Features

- **Non-blocking I/O**: Uses `poll`, `select`, `epoll`, or `kqueue` for scalable event-driven I/O.
- **Multiple Ports/Virtual Hosts**: Serve multiple domains and ports.
- **Configurable via File**: NGINX-like config for ports, hosts, server names, locations, error pages, size limits, allowed methods, and more.
- **CGI Support**: Run scripts (e.g., PHP, Python) for dynamic content.
- **HTTP Methods**: Implements `GET`, `POST`, and `DELETE`.
- **Directory Listing**: Optional, per route.
- **Custom/Error Pages**: Configurable per location.
- **File Uploads**: Via POST.
- **Static & Dynamic Content**: Serve HTML, CSS, JS, files, or runtimes via CGI.
- **Standards Compliance**: Accurate HTTP status codes, request/response headers.
- **Robust and Resilient**: Handles stress, avoids blocking, never hangs or crashes unexpectedly.
- **MacOS & Linux Support**
