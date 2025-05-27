#!/bin/bash
# Run ShipNetSim CLI with proper argument handling and defaults

# Default paths
DATA_DIR="$(pwd)/data"
OUTPUT_DIR="$(pwd)/output"

# Create directory structure if it doesn't exist
create_directory_structure() {
    echo "Setting up directory structure..."
    
    # Create main directories
    mkdir -p "$DATA_DIR"
    mkdir -p "$OUTPUT_DIR"
    
    # Create subdirectories for organization
    mkdir -p "$DATA_DIR/ships"
    mkdir -p "$DATA_DIR/boundaries"
    mkdir -p "$DATA_DIR/msc"
    
    # Create a README file to help users understand the structure
    if [[ ! -f "$DATA_DIR/README.md" ]]; then
        cat > "$DATA_DIR/README.md" << 'EOF'
# ShipNetSim Data Directory

This directory is for your local input files that will be used with ShipNetSim.

## Directory Structure:
- `ships/` - Place your ship configuration files (.dat format) here
- `boundaries/` - Place your water boundary files (.shp format) here  
- `msc/` - Place any other files here

## Usage Examples:
- Ship files: `./data/ships/myFleet.dat`
- Boundary files: `./data/boundaries/customOcean.shp`
- Mixed usage: Use `image:filename` for built-in files, `./data/path/file` for your files

## Built-in Files (accessible via image: prefix):
- `image:sampleShip.dat` - Sample ship configuration
- `default` - Default ocean boundaries (ne_110m_ocean.shp)

Run `./docker/scripts/run-cli.sh --help` for more information.
EOF
        echo "  Created README.md in $DATA_DIR with usage instructions"
    fi
    
    # Create a sample .gitignore to help with version control
    if [[ ! -f "$DATA_DIR/.gitignore" ]]; then
        cat > "$DATA_DIR/.gitignore" << 'EOF'
# Ignore large data files by default
*.shp
*.shx
*.dbf
*.prj

# But keep example/sample files
!*sample*
!*example*

# Keep the README
!README.md
!.gitignore
EOF
        echo "  Created .gitignore in $DATA_DIR"
    fi
    
    echo "  ✓ Created data directory: $DATA_DIR"
    echo "  ✓ Created output directory: $OUTPUT_DIR"
    echo "  ✓ Created subdirectories: ships/, boundaries/, msc/"
    echo ""
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [ShipNetSim options]"
    echo ""
    echo "This script runs ShipNetSim in a Docker container with proper volume mounting."
    echo "File paths will be automatically mapped to container paths."
    echo ""
    echo "Directory Structure:"
    echo "  ./data/ships/      - Your ship configuration files (.dat)"
    echo "  ./data/boundaries/ - Your water boundary files (.shp)"
    echo "  ./data/msc/    - Other files"
    echo "  ./output/          - Results and output files"
    echo ""
    echo "Required:"
    echo "  -s, --ships-file <file>     Ships configuration file (.dat format) (REQUIRED)"
    echo ""
    echo "Optional (with defaults):"
    echo "  -b, --water-boundaries-file Water boundaries file (default: ./data/ne_110m_ocean.shp)"
    echo "  -o, --output-folder         Output directory (default: ./output)"
    echo "  -r, --result-summaries      Summary filename (default: shipSummary_TIMESTAMP.txt)"
    echo ""
    echo "Examples:"
    echo "  $0 -s ./data/ships/myFleet.dat"
    echo "  $0 -s ./data/ships/fleet.dat -b ./data/boundaries/custom.shp"
    echo "  $0 -s image:sampleShip.dat -b default  # Use built-in files"
    echo "  $0 -s image:sampleShip.dat -b ./data/boundaries/custom.shp  # Mixed usage"
    echo ""
    echo "File Sources:"
    echo "  ./data/path/file   - Use your local files from ./data directory"
    echo "  image:filename     - Use built-in files from Docker image"
    echo "  default           - Use default boundary file from image"
    echo ""
    echo "Note: Run this script once to create the directory structure automatically."
}

# Check if help is requested
if [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]] || [[ "$1" == "-?" ]]; then
    show_usage
    echo ""
    echo "ShipNetSim help:"
    create_directory_structure
    docker run --rm \
        -v "$DATA_DIR":/app/local_data \
        -v "$OUTPUT_DIR":/app/output \
        shipnetsim:latest \
        ./ShipNetSim --help
    exit 0
fi

# Create directory structure on first run or if directories don't exist
if [[ ! -d "$DATA_DIR" ]] || [[ ! -d "$OUTPUT_DIR" ]]; then
    create_directory_structure
elif [[ ! -f "$DATA_DIR/README.md" ]]; then
    echo "Updating directory structure..."
    create_directory_structure
fi

# Initialize flags to track required/optional parameters
SHIPS_FILE_PROVIDED=false
BOUNDARIES_FILE_PROVIDED=false
OUTPUT_FOLDER_PROVIDED=false
RESULT_SUMMARIES_PROVIDED=false

# Generate timestamp for default summary filename
TIMESTAMP=$(date +"%Y%m%d_%H%M%S")
DEFAULT_SUMMARY_FILE="shipSummary_${TIMESTAMP}.txt"

# Process arguments to check what's provided and map paths
PROCESSED_ARGS=()
NEXT_IS_FILE=false
FILE_PARAM=""

for arg in "$@"; do
    if $NEXT_IS_FILE; then
        # This argument is a file path for the previous parameter
        case "$FILE_PARAM" in
            -s|--ships-file)
                SHIPS_FILE_PROVIDED=true
                if [[ "$arg" == "image:"* ]]; then
                    # Handle image files like "image:sampleShip.dat"
                    IMAGE_FILE="${arg#image:}"
                    CONTAINER_PATH="/app/data/$IMAGE_FILE"
                elif [[ "$arg" == ./data/* ]]; then
                    # Convert ./data/path/to/file.dat to /app/local_data/path/to/file.dat
                    RELATIVE_PATH="${arg#./data/}"
                    CONTAINER_PATH="/app/local_data/${RELATIVE_PATH}"
                elif [[ "$arg" == ./* ]]; then
                    # Handle other ./ paths by assuming they should go to local_data
                    RELATIVE_PATH="${arg#./}"
                    CONTAINER_PATH="/app/local_data/${RELATIVE_PATH}"
                else
                    CONTAINER_PATH="$arg"
                fi
                PROCESSED_ARGS+=("$CONTAINER_PATH")
                ;;
            -b|--water-boundaries-file)
                BOUNDARIES_FILE_PROVIDED=true
                if [[ "$arg" == "image:"* ]]; then
                    # Handle image files
                    IMAGE_FILE="${arg#image:}"
                    CONTAINER_PATH="/app/data/$IMAGE_FILE"
                elif [[ "$arg" == "default" ]]; then
                    # Special case for default boundary file (should be in image)
                    CONTAINER_PATH="default"
                elif [[ "$arg" == ./data/* ]]; then
                    # Convert ./data/path/to/file.shp to /app/local_data/path/to/file.shp
                    RELATIVE_PATH="${arg#./data/}"
                    CONTAINER_PATH="/app/local_data/${RELATIVE_PATH}"
                elif [[ "$arg" == ./* ]]; then
                    # Handle other ./ paths by assuming they should go to local_data
                    RELATIVE_PATH="${arg#./}"
                    CONTAINER_PATH="/app/local_data/${RELATIVE_PATH}"
                else
                    CONTAINER_PATH="$arg"
                fi
                PROCESSED_ARGS+=("$CONTAINER_PATH")
                ;;
            -o|--output-folder)
                OUTPUT_FOLDER_PROVIDED=true
                PROCESSED_ARGS+=("/app/output")
                ;;
            -r|--result-summaries)
                RESULT_SUMMARIES_PROVIDED=true
                PROCESSED_ARGS+=("/app/output/$(basename "$arg")")
                ;;
            -i|--insta-file)
                if [[ "$arg" == ./* ]]; then
                    CONTAINER_PATH="/app/output/$(basename "$arg")"
                else
                    CONTAINER_PATH="/app/output/$arg"
                fi
                PROCESSED_ARGS+=("$CONTAINER_PATH")
                ;;
            *)
                PROCESSED_ARGS+=("$arg")
                ;;
        esac
        NEXT_IS_FILE=false
        FILE_PARAM=""
    else
        # Check if this parameter expects a file path next
        case "$arg" in
            -s|--ships-file|-b|--water-boundaries-file|-o|--output-folder|-r|--result-summaries|-i|--insta-file)
                PROCESSED_ARGS+=("$arg")
                NEXT_IS_FILE=true
                FILE_PARAM="$arg"
                ;;
            *)
                PROCESSED_ARGS+=("$arg")
                ;;
        esac
    fi
done

# Check if ships file is provided (REQUIRED)
if ! $SHIPS_FILE_PROVIDED; then
    echo "ERROR: Ships file is required!"
    echo "Please provide a ships file (.dat format) using -s or --ships-file option."
    echo ""
    echo "Examples:"
    echo "  $0 -s ./data/ships/myFleet.dat        # Use your local file"
    echo "  $0 -s image:sampleShip.dat           # Use built-in sample file"
    echo ""
    echo "Place your .dat files in: $DATA_DIR/ships/"
    echo "Use --help for more information."
    exit 1
fi

# Add default water boundaries file if not provided
if ! $BOUNDARIES_FILE_PROVIDED; then
    echo "No water boundaries file specified, using default: ne_110m_ocean.shp"
    PROCESSED_ARGS+=("-b" "default")
fi

# Add default output folder if not provided
if ! $OUTPUT_FOLDER_PROVIDED; then
    echo "No output folder specified, using default: ./output"
    PROCESSED_ARGS+=("-o" "/app/output")
fi

# Add default result summaries file if not provided
if ! $RESULT_SUMMARIES_PROVIDED; then
    echo "No result summaries file specified, using default: $DEFAULT_SUMMARY_FILE"
    PROCESSED_ARGS+=("-r" "$DEFAULT_SUMMARY_FILE")
fi

# Check if the default boundary file exists in the local data directory
if ! $BOUNDARIES_FILE_PROVIDED && [[ ! -f "$DATA_DIR/ne_110m_ocean.shp" ]]; then
    echo "INFO: Default boundary file not found in local data directory."
    echo "      Using built-in default boundary file from Docker image."
fi

# Build the command with proper quoting (only quote values, not flags)
CMD_ARGS=""
for arg in "${PROCESSED_ARGS[@]}"; do
    # Check if this is a flag (starts with -)
    if [[ "$arg" == -* ]]; then
        CMD_ARGS="$CMD_ARGS $arg"
    else
        CMD_ARGS="$CMD_ARGS \"$arg\""
    fi
done

# Run the container with processed arguments
echo "Running ShipNetSim with the following configuration:"
echo "  Local data directory: $DATA_DIR -> /app/local_data"
echo "  Image data directory: (built-in) -> /app/data"
echo "  Output directory: $OUTPUT_DIR -> /app/output"
echo "  Arguments: ${PROCESSED_ARGS[*]}"
echo "  Command: ./ShipNetSim$CMD_ARGS"
echo ""

# Always mount both local_data and output directories
docker run --rm -it \
    -v "$DATA_DIR":/app/local_data \
    -v "$OUTPUT_DIR":/app/output \
    --name shipnetsim-cli \
    shipnetsim:latest \
    sh -c "./ShipNetSim$CMD_ARGS"