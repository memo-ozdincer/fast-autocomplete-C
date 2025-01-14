```markdown
# Autocomplete System

A C implementation of an autocomplete system that suggests terms based on prefix matching. The system uses binary search for efficient prefix matching and sorts results by weight.

## Features

- Reads terms and weights from a text file
- Performs case-sensitive prefix matching
- Uses binary search for O(log n) time complexity
- Returns matches sorted by weight in descending order
- Handles various edge cases and error conditions

## File Structure

- `main.c` - Main program file with example usage
- `autocomplete.h` - Header file with struct and function declarations
- `autocomplete.c` - Implementation of the autocomplete system
- `cities.txt` - Sample input file (you need to create this)

## Usage

1. Prepare an input file (e.g., `cities.txt`) with the following format:
   ```
   number_of_terms
   weight1 term1
   weight2 term2
   ...
   ```

2. Compile the program:
   ```bash
   gcc -o autocomplete main.c autocomplete.c
   ```

3. Run the program:
   ```bash
   ./autocomplete
   ```

## Functions

- `read_in_terms()`: Reads terms from file and sorts them lexicographically
- `lowest_match()`: Finds the first index of terms matching the prefix
- `highest_match()`: Finds the last index of terms matching the prefix
- `autocomplete()`: Returns matching terms sorted by weight

## Error Handling

- Handles file open/read errors
- Manages memory allocation failures
- Validates input parameters
- Handles malformed input files
```