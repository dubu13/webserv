NAME = webserv

CXX = c++
CXXFLAGS = -std=c++17 -Wall -Wextra -Werror -g3

SRC_DIR = src
OBJ_DIR = obj
INCLUDES = -I ./inc -I ./inc/server -I ./inc/HTTP -I ./inc/config -I ./inc/resource -I ./inc/errors -I ./inc/utils

# Colors
DEFAULT = "\033[0m"
GREEN = "\033[0;32m"
RED = "\033[0;31m"

# Find all cpp files recursively
SRCS = $(shell find $(SRC_DIR) -name "*.cpp" -type f)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

ifdef DEBUG
    CXXFLAGS += -DDEBUG_LOGGING
endif

all: $(NAME)

.SILENT:

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(NAME) $(OBJS)
	echo $(GREEN)"Building $(NAME)..."$(DEFAULT)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)
	echo $(RED)"Removing objects..."$(DEFAULT)

fclean: clean
	rm -f $(NAME)
	echo $(RED)"Removing $(NAME)..."$(DEFAULT)

re: fclean all

.PHONY: all clean fclean re
