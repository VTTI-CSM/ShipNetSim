#!/bin/bash
# Run ShipNetSim CLI with proper argument handling and defaults

# Default paths
DATA_DIR="$(pwd)/data"
OUTPUT_DIR="$(pwd)/output"

# Create output directory if it doesn't exist
mkdir -p "$OUTPUT_DIR"

# Function to show usage
show_usage() {
    echo "Usage: $0 [ShipNetSim options]"
    echo ""
    echo "This script runs ShipNetSim in a Docker container with proper volume mounting."
    echo "File paths will be automatically mapped to container paths."
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
    echo "  $0 -s ./data/ships.dat"
    echo "  $0 -s ./data/subfolder/my_fleet.dat -b ./data/boundaries/custom_boundaries.shp"
    echo "  $0 -s ./data/ships.dat -o ./custom_output -t 0.5"
    echo ""
    echo "Note: Relative paths starting with ./ will be mapped to container paths preserving full structure."
}

# Check if help is requested
if [[ "$1" == "--help" ]] || [[ "$1" == "-h" ]] || [[ "$1" == "-?" ]]; then
    show_usage
    echo ""
    echo "ShipNetSim help:"
    docker run --rm \
        -v "$DATA_DIR":/app/data \
        -v "$OUTPUT_DIR":/app/output \
        shipnetsim:latest \
        ./ShipNetSim --help
    exit 0
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
                if [[ "$arg" == ./data/* ]]; then
                    # Convert ./data/path/to/file.dat to /app/data/path/to/file.dat
                    RELATIVE_PATH="${arg#./data/}"
                    CONTAINER_PATH="/app/data/${RELATIVE_PATH}"
                elif [[ "$arg" == ./* ]]; then
                    # Handle other ./ paths by assuming they should go to data
                    RELATIVE_PATH="${arg#./}"
                    CONTAINER_PATH="/app/data/${RELATIVE_PATH}"
                else
                    CONTAINER_PATH="$arg"
                fi
                PROCESSED_ARGS+=("$CONTAINER_PATH")
                ;;
            -b|--water-boundaries-file)
                BOUNDARIES_FILE_PROVIDED=true
                if [[ "$arg" == ./data/* ]]; then
                    # Convert ./data/path/to/file.shp to /app/data/path/to/file.shp
                    RELATIVE_PATH="${arg#./data/}"
                    CONTAINER_PATH="/app/data/${RELATIVE_PATH}"
                elif [[ "$arg" == ./* ]]; then
                    # Handle other ./ paths by assuming they should go to data
                    RELATIVE_PATH="${arg#./}"
                    CONTAINER_PATH="/app/data/${RELATIVE_PATH}"
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
    echo "Example: $0 -s ./data/ships.dat"
    echo ""
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
    PROCESSED_ARGS+=("-r" "/app/output/$DEFAULT_SUMMARY_FILE")
fi

# Check if the default boundary file exists in the data directory
if ! $BOUNDARIES_FILE_PROVIDED && [[ ! -f "$DATA_DIR/ne_110m_ocean.shp" ]]; then
    echo "WARNING: Default boundary file '$DATA_DIR/ne_110m_ocean.shp' not found!"
    echo "Make sure the file exists or specify a custom boundary file with -b option."
fi

# Run the container with processed arguments
echo "Running ShipNetSim with the following configuration:"
echo "  Data directory: $DATA_DIR -> /app/data"
echo "  Output directory: $OUTPUT_DIR -> /app/output"
echo "  Arguments: ${PROCESSED_ARGS[*]}"
echo ""

docker run --rm -it \
    -v "$DATA_DIR":/app/data \
    -v "$OUTPUT_DIR":/app/output \
    --name shipnetsim-cli \
    shipnetsim:latest \
    ./ShipNetSim "${PROCESSED_ARGS[@]}"