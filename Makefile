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

# tests: every .cpp under tests/ + the whole server (minus main.cpp) so test
# mains can link against Server/Client/Command without duplicate main()
TEST_DIR= $(CWD)/tests
TEST_BIN= $(BUILD_DIR)/run_tests
TEST_SRCS= $(shell find $(TEST_DIR) -type f -name "*.cpp")
LIB_SRCS= $(filter-out $(SRC_DIR)/main.cpp, $(SRCS))

all: $(BUILD_DIR) $(NAME) $(HEADERS)

$(NAME): $(OBJS) $(HEADERS)
	$(CXX) $(CXXCFLAGS) $(INCLUDES) $(OBJS) -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXCFLAGS) $(INCLUDES) -c $< -o $@

test: $(BUILD_DIR)
	$(CXX) $(CXXCFLAGS) $(INCLUDES) $(TEST_SRCS) $(LIB_SRCS) -o $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -rf $(BUILD_DIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all re clean fclean test