NAME = webserv

CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -g3

SRC_DIR = src
OBJ_DIR = obj
INCLUDES = -I ./inc -I ./inc/server

SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/server/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

all: $(NAME)

.SILENT:

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	echo $(GREEN)"Building $(NAME)..."$(DEFAULT);

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJS)
	echo $(RED)"Removing $(NAME) objects..."$(DEFAULT);

fclean: clean
	rm -f $(NAME)
	rm -rf $(OBJ_DIR)
	echo $(RED)"Removing $(NAME)..."$(DEFAULT);

re: fclean all

.PHONY: all clean fclean re

# Colors
DEFAULT = "\033[0m"
GREEN = "\033[0;32m"
RED = "\033[0;31m"