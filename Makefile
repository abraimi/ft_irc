NAME= ft_irc

CXX= c++
CXXCFLAGS= #-Wall -Wextra -Werror -std=c++98
CXXBFLAGS=

CWD= .
SRC_DIR= $(CWD)/src
BUILD_DIR= build

INCLUDES_DIR= $(CWD)/include
INCLUDES= -I$(CWD)/include

SRCS= $(shell find $(SRC_DIR) -type f -name "*.cpp")
OBJS= $(addprefix $(BUILD_DIR)/, $(SRCS:$(SRC_DIR)/%.cpp=%.o))
HEADERS= $(shell find $(INCLUDES_DIR) -type f -name "*.hpp")

all: $(BUILD_DIR) $(NAME) $(HEADERS)

$(NAME): $(OBJS) $(HEADERS)
	$(CXX) $(CXXCFLAGS) $(INCLUDES) $(OBJS) -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXCFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all re clean fclean