#!/bin/bash

# Configuration
TEST_DIR="tests"
COMPILER_NAME="my_compiler"

# 1. Clean up old executable and Mac debug folders
echo "🧹 Cleaning old files..."
rm -f $COMPILER_NAME
rm -rf $COMPILER_NAME.dSYM

# 2. Compile the project
echo "🔨 Compiling new executable..."
gcc -Wall -o $COMPILER_NAME main.c lexer.c parser.c

# Check if compilation succeeded
if [ $? -ne 0 ]; then
    echo "Compilation failed! Fix C code errors in main/lexer/parser."
    exit 1
fi

echo "Compilation successful. Running tests 0-9 in /$TEST_DIR..."
echo "---------------------------------------"

# 3. Loop through files 0.c to 9.c
for i in {0..9}
do
    FILE="$TEST_DIR/$i.c"
    
    if [ -f "$FILE" ]; then
        echo "📄 Testing $FILE:"
        
        # Run your compiler on the test file
        ./$COMPILER_NAME "$FILE"
        
        # Capture exit status
        # (Assuming your tkerr() calls exit(-1) on failure)
        if [ $? -eq 0 ]; then
            echo "PASSED"
        else
            echo "FAILED"
        fi
        echo "---------------------------------------"
    else
        echo "$FILE not found, skipping."
        echo "---------------------------------------"
    fi
done

echo "Done."