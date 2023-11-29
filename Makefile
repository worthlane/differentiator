CXX = g++-13
EXECUTABLE = diff
CXXFLAGS  = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations \
			-Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts       \
			-Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal  \
			-Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline          \
			-Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked            \
			-Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo  \
			-Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn                \
			-Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default      \
			-Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast           \
			-Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing   \
			-Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation    \
			-fstack-protector -fstrict-overflow -fno-omit-frame-pointer -Wlarger-than=8192         \
			-Wstack-usage=8192 -fPIE -Werror=vla

HOME = $(shell pwd)
CXXFLAGS += -I $(HOME)

IMAGE = img
BUILD_DIR = build/bin
OBJECTS_DIR = build
SOURCES = main.cpp calculation.cpp
EXPRESSION_SOURCES = expression.cpp visual.cpp expr_input_and_output.cpp
EXPRESSION_DIR = expression
COMMON_SOURCES = logs.cpp errors.cpp input_and_output.cpp file_read.cpp
COMMON_DIR = common
OBJECTS = $(SOURCES:%.cpp=$(OBJECTS_DIR)/%.o)
EXPRESSION_OBJECTS = $(EXPRESSION_SOURCES:%.cpp=$(OBJECTS_DIR)/%.o)
COMMON_OBJECTS = $(COMMON_SOURCES:%.cpp=$(OBJECTS_DIR)/%.o)
DOXYFILE = Doxyfile
DOXYBUILD = doxygen $(DOXYFILE)

.PHONY: all
all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) $(EXPRESSION_OBJECTS) $(COMMON_OBJECTS)
	$(CXX) $^ -o $@ $(CXXFLAGS)

$(OBJECTS_DIR)/%.o : %.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)

$(OBJECTS_DIR)/%.o : $(COMMON_DIR)/%.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)

$(OBJECTS_DIR)/%.o : $(EXPRESSION_DIR)/%.cpp
	$(CXX) -c $^ -o $@ $(CXXFLAGS)

.PHONY: doxybuild clean install test

doxybuild:
	$(DOXYBUILD)

clean:
	rm -rf $(EXECUTABLE) $(OBJECTS_DIR)/*.o *.html *.log $(IMAGE)/*.png *.dot

makedirs:
	mkdir -p $(BUILD_DIR)
	mkdir -p $(IMAGE)

test:
	echo $(OBJECTS) $(STACK_OBJECTS)
