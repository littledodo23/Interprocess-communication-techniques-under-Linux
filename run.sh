cat > run.sh << 'EOF'
#!/bin/bash

# ===================================================
# Run script for Genetic Algorithm Rescue Operations
# ===================================================

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}================================${NC}"
echo -e "${GREEN}Genetic Rescue Operations${NC}"
echo -e "${GREEN}================================${NC}"
echo ""

# Check if executable exists
if [ ! -f "./rescue_ga" ]; then
    echo -e "${YELLOW}Executable not found. Building project...${NC}"
    make
    if [ $? -ne 0 ]; then
        echo -e "${RED}Build failed! Please check for errors.${NC}"
        exit 1
    fi
    echo ""
fi

# Check if config file exists
CONFIG_FILE="config/config.txt"
if [ ! -f "$CONFIG_FILE" ]; then
    echo -e "${YELLOW}Warning: Config file not found${NC}"
    echo -e "${YELLOW}Using default values...${NC}"
    CONFIG_FILE=""
fi

# Run the program
echo -e "${GREEN}Running program...${NC}"
echo ""

if [ -z "$CONFIG_FILE" ]; then
    ./rescue_ga
else
    ./rescue_ga $CONFIG_FILE
fi

EXIT_CODE=$?

echo ""
if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}Program completed successfully!${NC}"
    
    if [ -f "output/results.txt" ]; then
        echo -e "${GREEN}Results: output/results.txt${NC}"
    fi
    
    if [ -f "output/grid_layout.txt" ]; then
        echo -e "${GREEN}Grid: output/grid_layout.txt${NC}"
    fi
else
    echo -e "${RED}Error code: $EXIT_CODE${NC}"
fi

echo ""
echo -e "${GREEN}================================${NC}"
EOF