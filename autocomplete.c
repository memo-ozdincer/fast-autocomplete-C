#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "autocomplete.h"

/*
 * Helper function to compare two terms lexicographically (ascending).
 * Used by qsort in read_in_terms().
 */
static int compare_lex(const void *a, const void *b)
{
    const term *t1 = (const term *)a;
    const term *t2 = (const term *)b;
    return strcmp(t1->term, t2->term);
}

/*
 * Helper function to compare two terms by weight (descending).
 * Used by qsort in autocomplete().
 */
static int compare_weight_desc(const void *a, const void *b)
{
    const term *t1 = (const term *)a;
    const term *t2 = (const term *)b;
    if (t2->weight > t1->weight) return 1;
    else if (t2->weight < t1->weight) return -1;
    else return 0;
}

/*
 * Checks if 'str' starts with 'prefix' (case-sensitive).
 * Returns 1 if yes, 0 if no.
 */
static int starts_with(const char *str, const char *prefix)
{
    while (*prefix) {
        if (*prefix != *str) {
            return 0;
        }
        prefix++;
        str++;
    }
    return 1;
}

/*
 * read_in_terms():
 *   - Reads the number of terms (first line in the file).
 *   - Allocates memory for that many terms.
 *   - Reads each line into the array, splitting weight from the string.
 *   - Sorts the array in lexicographically ascending order using qsort.
 *
 * Edge cases addressed:
 *   - If the file can't be opened, prints an error and sets *pnterms=0,*terms=NULL.
 *   - If the file format is malformed, attempts to skip or handle as many lines as possible.
 */
void read_in_terms(struct term **terms, int *pnterms, char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        *pnterms = 0;
        *terms = NULL;
        return;
    }

    // Read the number of terms
    if (fscanf(fp, "%d", pnterms) != 1 || *pnterms <= 0) {
        fprintf(stderr, "Error: Invalid format for number of terms in %s\n", filename);
        fclose(fp);
        *pnterms = 0;
        *terms = NULL;
        return;
    }

    // Allocate memory for all terms
    *terms = malloc(sizeof(struct term) * (*pnterms));
    if (!(*terms)) {
        fprintf(stderr, "Error: Could not allocate memory.\n");
        fclose(fp);
        *pnterms = 0;
        return;
    }

    // Read each term line:
    // Format assumed:  <weight><whitespace><term string (possibly containing spaces)>
    // Example:
    //    13076300   Buenos Aires, Argentina
    // We use fgets to ensure we read entire lines (safer than fscanf).
    // Then we parse the line with sscanf for the weight and the term string.

    // Discard any leftover newline from the line that contained the count
    fgetc(fp);

    for (int i = 0; i < *pnterms; i++) {
        char line[512];
        if (!fgets(line, sizeof(line), fp)) {
            // Unexpected end of file or read error
            fprintf(stderr, "Warning: early end of file at line %d\n", i+2);
            (*terms)[i].term[0] = '\0';
            (*terms)[i].weight = 0.0;
            continue;
        }

        // Trim leading/trailing whitespace
        // (Optional, but helps avoid parsing issues if there are strange spaces)
        // We can do a quick parse with sscanf.
        double weight = 0.0;
        char temp_string[300];
        // Because term can be up to 200 characters, we read up to 299 to be safe in the buffer:
        temp_string[0] = '\0';

        // Use two-step parse: first parse the weight, then parse the rest as a string.
        // Some lines might have leading spaces causing trouble, so we include them in format.
        // NOTE: If the line is badly malformed, weight might be 0.0 or the string might be empty.
        int count = sscanf(line, " %lf %[^\n]", &weight, temp_string);
        if (count < 2) {
            // Possibly malformed line; store something default
            fprintf(stderr, "Warning: malformed line %d in %s\n", i+2, filename);
            weight = 0.0;
            strcpy(temp_string, "");
        }

        (*terms)[i].weight = weight;
        // Truncate string if needed
        strncpy((*terms)[i].term, temp_string, sizeof((*terms)[i].term) - 1);
        (*terms)[i].term[sizeof((*terms)[i].term) - 1] = '\0';  // ensure NUL termination
    }

    fclose(fp);

    // Sort the array in lexicographically ascending order
    qsort(*terms, *pnterms, sizeof(struct term), compare_lex);
}

/*
 * lowest_match():
 *   - Performs a binary search for the first (lowest) index that starts with substr.
 *   - Returns -1 if no match is found.
 *
 * Requirements: O(log(nterms)) time complexity.
 *
 * Approach:
 *   - Use binary search boundaries to find the region containing substr.
 *   - We are effectively finding the left boundary of terms that start with substr.
 */
int lowest_match(struct term *terms, int nterms, char *substr)
{
    if (!terms || nterms <= 0 || !substr || substr[0] == '\0') {
        return -1; // No valid search
    }

    int left = 0;
    int right = nterms - 1;
    int result = -1;

    while (left <= right) {
        int mid = (left + right) / 2;
        if (starts_with(terms[mid].term, substr)) {
            // If this term starts with substr,
            // try to see if there's a lower index that also starts with substr
            result = mid;
            right = mid - 1;
        } else {
            // Compare lexicographically to decide which way to go
            // We can compare substr with the portion of terms[mid].term that is the same length.
            int cmp = strncmp(terms[mid].term, substr, strlen(substr));
            if (cmp < 0) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
    }

    return result;
}

/*
 * highest_match():
 *   - Performs a binary search for the last (highest) index that starts with substr.
 *   - Returns -1 if no match is found.
 *
 * Requirements: O(log(nterms)) time complexity.
 */
int highest_match(struct term *terms, int nterms, char *substr)
{
    if (!terms || nterms <= 0 || !substr || substr[0] == '\0') {
        return -1; // No valid search
    }

    int left = 0;
    int right = nterms - 1;
    int result = -1;

    while (left <= right) {
        int mid = (left + right) / 2;
        if (starts_with(terms[mid].term, substr)) {
            // If this term starts with substr,
            // we should see if there's a higher index that also starts with substr
            result = mid;
            left = mid + 1;
        } else {
            int cmp = strncmp(terms[mid].term, substr, strlen(substr));
            if (cmp < 0) {
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }
    }

    return result;
}

/*
 * autocomplete():
 *   - Finds all terms that start with substr (using lowest_match() and highest_match()).
 *   - Allocates a new array for *answer containing these matching terms.
 *   - Sorts these matching terms by weight in descending order (non-increasing).
 *   - Sets *n_answer to the count of matching items.
 *
 * Edge cases:
 *   - If no match, set *answer = NULL, *n_answer = 0.
 *   - If substr is empty, returns no matches by default or the entire array
 *     depending on interpretation. Here, we assume an empty substring
 *     trivially does not match any term; or you can choose to interpret
 *     it as “everything matches.” Clarify as needed.
 */
void autocomplete(struct term **answer, int *n_answer, struct term *terms, int nterms, char *substr)
{
    *answer = NULL;
    *n_answer = 0;

    // Edge case: no terms, or invalid substring
    if (!terms || nterms <= 0 || !substr || substr[0] == '\0') {
        return;
    }

    int low_idx = lowest_match(terms, nterms, substr);
    int high_idx = highest_match(terms, nterms, substr);

    // If either is -1, no match
    if (low_idx == -1 || high_idx == -1) {
        return;
    }

    int count = high_idx - low_idx + 1;
    if (count <= 0) {
        return;
    }

    // Allocate memory for the answer
    *answer = malloc(sizeof(struct term) * count);
    if (!(*answer)) {
        fprintf(stderr, "Error: Could not allocate memory for autocomplete list.\n");
        return;
    }

    // Copy matching terms
    for (int i = 0; i < count; i++) {
        (*answer)[i] = terms[low_idx + i];
    }

    // Sort by weight descending
    qsort(*answer, count, sizeof(struct term), compare_weight_desc);

    *n_answer = count;
}
