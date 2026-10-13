SERVER_NAME             =   zappy_server
GUI_NAME                =   zappy_gui
GUI3D_NAME              =   zappy_gui3d
AI_NAME                 =   zappy_ai
LAUNCHER_NAME           =   zappy_launcher
GUI_PROTOCOL_TEST_NAME  =   test_gui_protocol

BUILD_DIR   =   build

SRC_SERVER  =   $(shell find src/server src/map -name '*.cpp')
SRC_GUI     =   $(shell find src/gui -name '*.cpp')
SRC_AI      =   $(shell find src/agents -name '*.cpp')
SRC_MENU    =   $(shell find src/menu -name '*.cpp')
SRC_GUI3D   =   $(shell find src/gui3d -name '*.cpp') \
                $(shell find src/gui/network -name '*.cpp') \
                src/gui/graphical/world.cpp \
                src/gui/graphical/gui_protocol_handler.cpp \
                $(shell find src/gui/utils -name '*.cpp')
SRC_GUI_PROTOCOL_TEST   =   tests/gui_protocol_tests.cpp \
                            src/gui/graphical/world.cpp \
                            src/gui/graphical/gui_protocol_handler.cpp

OBJ_SERVER  =   $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_SERVER))
OBJ_GUI     =   $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_GUI))
OBJ_AI      =   $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_AI))
OBJ_MENU    =   $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_MENU))
OBJ_GUI3D   =   $(patsubst src/%.cpp,$(BUILD_DIR)/%.o,$(SRC_GUI3D))

DEP_SERVER  =   $(OBJ_SERVER:.o=.d)
DEP_GUI     =   $(OBJ_GUI:.o=.d)
DEP_AI      =   $(OBJ_AI:.o=.d)
DEP_MENU    =   $(OBJ_MENU:.o=.d)
DEP_GUI3D   =   $(OBJ_GUI3D:.o=.d)
DEPFILES    =   $(DEP_SERVER) $(DEP_GUI) $(DEP_AI) $(DEP_MENU) $(DEP_GUI3D)

LDFLAGS     =   -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
CXXFLAGS    =   -I./include -I./src/gui -I./src/gui/graphical \
                -I./src/gui/network -I./src/gui/utils \
                -Wall -Wextra -Wpedantic -std=c++23 -MMD -MP
TEST_CXXFLAGS   =   $(filter-out -MMD -MP,$(CXXFLAGS))
CCXX        =   clang++

MODE_FILE   =   $(BUILD_DIR)/.mode

all:     MODE = release
debug:   MODE = debug
verbose: MODE = verbose

debug:   CXXFLAGS += -DDEBUG -g3
verbose: CXXFLAGS += -DVERBOSE -DDEBUG -g3

all debug verbose: _check_mode build

gui3d_standalone: CXXFLAGS += -DSTANDALONE
gui3d_standalone:
	@$(MAKE) --no-print-directory $(GUI3D_NAME) CXXFLAGS="$(CXXFLAGS)"

_check_mode:
	@if [ ! -f $(MODE_FILE) ] || [ "$$(cat $(MODE_FILE) 2>/dev/null)" != "$(MODE)" ]; then \
		$(MAKE) --no-print-directory fclean; \
		mkdir -p $(BUILD_DIR); \
		echo "$(MODE)" > $(MODE_FILE); \
	fi

build:
	@$(MAKE) --no-print-directory $(SERVER_NAME)  CXXFLAGS="$(CXXFLAGS)"
	@$(MAKE) --no-print-directory $(GUI_NAME)     CXXFLAGS="$(CXXFLAGS)"
	@$(MAKE) --no-print-directory $(GUI3D_NAME)   CXXFLAGS="$(CXXFLAGS)"
	@$(MAKE) --no-print-directory $(AI_NAME)      CXXFLAGS="$(CXXFLAGS)"
	@$(MAKE) --no-print-directory $(LAUNCHER_NAME) CXXFLAGS="$(CXXFLAGS)"

$(SERVER_NAME): $(OBJ_SERVER)
	@$(CCXX) $(OBJ_SERVER) -o $(SERVER_NAME)
	@echo "\e[095mLinking server done\e[0m"

$(GUI_NAME): $(OBJ_GUI)
	@$(CCXX) $(OBJ_GUI) -o $(GUI_NAME) $(LDFLAGS)
	@echo "\e[095mLinking gui done\e[0m"

$(GUI3D_NAME): $(OBJ_GUI3D)
	@$(CCXX) $(OBJ_GUI3D) -o $(GUI3D_NAME) $(LDFLAGS)
	@echo "\e[095mLinking gui3d done\e[0m"

$(AI_NAME): $(OBJ_AI)
	@$(CCXX) $(OBJ_AI) -o $(AI_NAME) $(LDFLAGS)
	@echo "\e[095mLinking ai done\e[0m"

$(LAUNCHER_NAME): $(OBJ_MENU)
	@$(CCXX) $(OBJ_MENU) -o $(LAUNCHER_NAME) $(LDFLAGS)
	@echo "\e[095mLinking launcher done\e[0m"

$(GUI_PROTOCOL_TEST_NAME): $(SRC_GUI_PROTOCOL_TEST) Makefile
	@echo "\e[036mBuilding GUI protocol tests\e[0m"
	@$(CCXX) $(TEST_CXXFLAGS) $(SRC_GUI_PROTOCOL_TEST) -o $(GUI_PROTOCOL_TEST_NAME)
	@echo "\e[032mBuilt GUI protocol tests\e[0m"

tests: $(GUI_PROTOCOL_TEST_NAME)
	@./$(GUI_PROTOCOL_TEST_NAME)

$(BUILD_DIR)/%.o: src/%.cpp Makefile
	@mkdir -p $(dir $@)
	@echo "\e[036mBuilding $<\e[0m"
	@$(CCXX) $(CXXFLAGS) -c $< -o $@
	@echo "\e[032mBuilt $<\e[0m"

clean:
	@rm -rf $(BUILD_DIR)
	@find . -type f -name '*.pch' -delete
	@echo "\e[031mCleaning done\e[0m"

fclean: clean
	@rm -rf $(SERVER_NAME)
	@rm -rf $(GUI_NAME)
	@rm -rf $(GUI3D_NAME)
	@rm -rf $(AI_NAME)
	@rm -rf $(LAUNCHER_NAME)
	@rm -rf $(GUI_PROTOCOL_TEST_NAME)
	@rm -rf logs

re: fclean
	@$(MAKE) --no-print-directory all

.PHONY: all debug verbose tests clean fclean re build _check_mode gui3d_standalone \
        $(SERVER_NAME) $(GUI_NAME) $(GUI3D_NAME) $(AI_NAME) $(LAUNCHER_NAME)

-include $(DEPFILES)
