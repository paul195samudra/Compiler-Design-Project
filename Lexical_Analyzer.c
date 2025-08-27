#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINE 1000
#define MAX_TOKENS_PER_LINE 200

// Symbol Table Struct
typedef struct {
    char name[100];    // Identifier name
    char type[100];    // Data type (int, float, etc.)
    char value[100];   // Value or "-" if uninitialized
    int line;          // Line number declared
} Symbol;

Symbol symbolTable[1000];
int symbolCount = 0;

//Valid/Invalid Identifiers
char validIdentifiers[1000][100];
int validIdentifiersCount = 0;
char invalidIdentifiers[1000][100];
int invalidIdentifiersCount = 0;

//Other Tokens
char othersFound[1000][100];
int othersCount = 0;

// Keyword List
const char *keywords[] = {
    "int", "float", "char", "double", "return", "if", "else", "for", "while",
    "void", "do", "switch", "case", "default", "break", "continue", "struct",
    "typedef", "include", "define", "unsigned", "const", "static", "long", "short", "signed"
};
int keywordCount = sizeof(keywords) / sizeof(keywords[0]);

//Multi-character Operators
const char *multiCharOps[] = {
    "++", "--", "==", "!=", "<=", ">=", "&&", "||", "+=", "-=", "*=", "/="
};

// Utility Functions

int isKeyword(const char *word) {
    for (int i = 0; i < keywordCount; i++) {
        if (strcmp(word, keywords[i]) == 0)
            return 1;
    }
    return 0;
}

int isMultiCharOp(const char *word) {
    for (int i = 0; i < sizeof(multiCharOps) / sizeof(multiCharOps[0]); i++) {
        if (strcmp(word, multiCharOps[i]) == 0)
            return 1;
    }
    return 0;
}

int isOperatorChar(char ch) {
    return strchr("+-*/%=<>!&|^~", ch) != NULL;
}

int isOperatorString(const char *str) {
    if (strlen(str) == 1 && isOperatorChar(str[0])) return 1;
    if (isMultiCharOp(str)) return 1;
    return 0;
}

int isBracket(char ch) {
    return strchr("(){}[]", ch) != NULL;
}

int isSeparator(char ch) {
    return strchr(",;:", ch) != NULL;
}

int isSpecialSymbol(char ch) {
    return strchr("#.", ch) != NULL;
}

// Checks if token is a valid data type token (for declaration parsing)
int isDataTypeToken(const char *token) {
    const char *dataTypes[] = {
        "int", "float", "char", "double", "void",
        "unsigned", "const", "static", "long", "short", "signed"
    };
    int count = sizeof(dataTypes) / sizeof(dataTypes[0]);
    for (int i = 0; i < count; i++) {
        if (strcmp(token, dataTypes[i]) == 0)
            return 1;
    }
    return 0;
}

//Advanced Custom Regex Matcher
// Pattern:
// - Optional leading '#', '@', or '!'
// - 4 to 7 lowercase letters (a-z), no more than two consecutive same letters
// - 2 to 4 digits (0-9), no negative, no more than two consecutive same digits
// - Ends with "@r"

bool isValidIdentifier_Advanced(const char *str) {
    int i = 0, len = strlen(str);

    if (len < 8 || len > 14) return false; // Min: 4 letters + 2 digits + @r = 8, Max: 1 + 7 letters + 4 digits + @r = 14

    // Optional leading '#', '@', or '!'
    if (str[i] == '#' || str[i] == '@' || str[i] == '!') i++;

    // 4 to 7 lowercase letters (a-z), no more than two consecutive same letters
    int letterCount = 0;
    char prev_letter = '\0';
    int consec_letter = 0;
    while (i < len && islower(str[i])) {
        letterCount++;
        if (letterCount == 1) {
            prev_letter = str[i];
            consec_letter = 1;
        } else {
            if (str[i] == prev_letter) {
                consec_letter++;
                if (consec_letter > 2) return false;
            } else {
                consec_letter = 1;
                prev_letter = str[i];
            }
        }
        i++;
    }
    if (letterCount < 4 || letterCount > 7) return false;

    // 2 to 4 digits (0-9), no more than two consecutive same digits
    int digitCount = 0;
    char prev_digit = '\0';
    int consec_digit = 0;
    while (i < len && isdigit(str[i])) {
        digitCount++;
        if (digitCount == 1) {
            prev_digit = str[i];
            consec_digit = 1;
        } else {
            if (str[i] == prev_digit) {
                consec_digit++;
                if (consec_digit > 2) return false;
            } else {
                consec_digit = 1;
                prev_digit = str[i];
            }
        }
        i++;
    }
    if (digitCount < 2 || digitCount > 4) return false;

    // Must end with "@r"
    if (i + 1 >= len || strncmp(&str[i], "@r", 2) != 0) return false;

    // Ensure we consumed the entire string
    return i + 2 == len;
}

// Symbol Table Functions

int alreadyInSymbolTable(const char *name) {
    for (int i = 0; i < symbolCount; i++) {
        if (strcmp(symbolTable[i].name, name) == 0)
            return 1;
    }
    return 0;
}

void addToSymbolTable(const char *type, const char *name, const char *value, int line) {
    if (!alreadyInSymbolTable(name)) {
        strncpy(symbolTable[symbolCount].type, type, sizeof(symbolTable[symbolCount].type)-1);
        strncpy(symbolTable[symbolCount].name, name, sizeof(symbolTable[symbolCount].name)-1);
        strncpy(symbolTable[symbolCount].value, value, sizeof(symbolTable[symbolCount].value)-1);
        symbolTable[symbolCount].line = line;
        symbolCount++;
    }
}

// Tokenization Helper

int tokenizeLine(char *line, char tokens[][100]) {
    int tokenIndex = 0;
    int i = 0;
    int len = strlen(line);
    while (i < len) {
        // Skip whitespace
        if (isspace(line[i])) {
            i++;
            continue;
        }

        // Handle multi-char operators (check first 2 chars)
        if (i + 1 < len) {
            char twoChar[3] = {line[i], line[i+1], '\0'};
            if (isMultiCharOp(twoChar)) {
                strcpy(tokens[tokenIndex++], twoChar);
                i += 2;
                continue;
            }
        }

        // Single char operators, separators, brackets
        if (isOperatorChar(line[i]) || isSeparator(line[i]) || isBracket(line[i]) || isSpecialSymbol(line[i])) {
            char oneChar[2] = {line[i], '\0'};
            strcpy(tokens[tokenIndex++], oneChar);
            i++;
            continue;
        }

        // Otherwise read a word/identifier/number literal
        int start = i;
        if (isalpha(line[i]) || line[i] == '_' || line[i] == '#' || line[i] == '@' || line[i] == '!') {
            // Identifier or keyword
            while (i < len && (isalnum(line[i]) || line[i] == '_' || line[i] == '@' || line[i] == '!')) i++;
        } else if (isdigit(line[i])) {
            // Number literal (integer or float)
            while (i < len && (isdigit(line[i]) || line[i] == '.')) i++;
        } else if (line[i] == '\"') {
            // String literal - read until closing quote
            i++; // skip opening quote
            while (i < len && line[i] != '\"') i++;
            if (i < len) i++; // skip closing quote
        } else if (line[i] == '\'') {
            // Character literal
            i++; // skip opening quote
            if (i < len && line[i] != '\'') i++; // skip char
            if (i < len && line[i] == '\'') i++; // skip closing quote
        } else {
            // Unknown single char token
            char oneChar[2] = {line[i], '\0'};
            strcpy(tokens[tokenIndex++], oneChar);
            i++;
            continue;
        }
        int length = i - start;
        if (length > 0) {
            strncpy(tokens[tokenIndex], &line[start], length);
            tokens[tokenIndex][length] = '\0';
            tokenIndex++;
        }
    }
    return tokenIndex;
}

// ==================== Processing Declarations ====================

void processDeclarationTokens(char tokens[][100], int startIndex, int tokenCount, const char *fullType, int lineno) {
    int i = startIndex;
    while (i < tokenCount) {
        // Skip commas
        if (strcmp(tokens[i], ",") == 0) {
            i++;
            continue;
        }
        // End if semicolon
        if (strcmp(tokens[i], ";") == 0) break;

        char varName[100] = "";
        char varValue[100] = "-";

        // The next token should be a potential identifier (variable name)
        if (isValidIdentifier_Advanced(tokens[i])) {
            strcpy(varName, tokens[i]);
            // Add to valid identifiers
            int found = 0;
            for (int k = 0; k < validIdentifiersCount; k++) {
                if (strcmp(validIdentifiers[k], varName) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(validIdentifiers[validIdentifiersCount++], varName);
            }
            i++;

            // Check if initialization: =
            if (i < tokenCount && strcmp(tokens[i], "=") == 0) {
                i++;
                if (i < tokenCount) {
                    strcpy(varValue, tokens[i]);
                    i++;
                }
            }

            addToSymbolTable(fullType, varName, varValue, lineno);
        } else {
            // Only add to invalid identifiers if it could be a variable name
            if (!isKeyword(tokens[i]) && !isOperatorString(tokens[i]) && !isBracket(tokens[i][0]) &&
                !isSeparator(tokens[i][0]) && !isSpecialSymbol(tokens[i][0]) && !isdigit(tokens[i][0]) &&
                tokens[i][0] != '"' && tokens[i][0] != '\'') {
                int found = 0;
                for (int k = 0; k < invalidIdentifiersCount; k++) {
                    if (strcmp(invalidIdentifiers[k], tokens[i]) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(invalidIdentifiers[invalidIdentifiersCount++], tokens[i]);
                }
            }
            i++;
        }
    }
}

// ==================== Main Lexical Analyzer ====================

void processFile(FILE *input, FILE *output) {
    char line[MAX_LINE];
    int lineno = 0;

    // To accumulate all tokens for token categories output
    char keywordsFound[1000][100];
    int keywordsCount = 0;

    char numericsFound[1000][100];
    int numericsCount = 0;

    char stringLiteralsFound[1000][1000];
    int stringLiteralsCount = 0;

    char multiCharOpsFound[1000][10];
    int multiCharOpsCount = 0;

    char operatorsFound[1000][10];
    int operatorsCount = 0;

    char separatorsFound[1000][10];
    int separatorsCount = 0;

    char bracketsFound[1000][10];
    int bracketsCount = 0;

    char specialSymbolsFound[1000][10];
    int specialSymbolsCount = 0;

    // Multi-line comment handling
    int insideComment = 0;

    while (fgets(line, sizeof(line), input)) {
        lineno++;

        // Remove newline at end
        line[strcspn(line, "\n")] = 0;

        // Handle multi-line comments /* ... */
        if (insideComment) {
            char *endComment = strstr(line, "*/");
            if (endComment) {
                insideComment = 0;
                memmove(line, endComment + 2, strlen(endComment + 2) + 1);
            } else {
                continue;
            }
        }
        char *startComment = strstr(line, "/*");
        if (startComment) {
            insideComment = 1;
            *startComment = '\0';
        }

        // Remove single-line comments (//)
        char *comment = strstr(line, "//");
        if (comment) *comment = '\0';

        // Tokenize line
        char tokens[MAX_TOKENS_PER_LINE][100];
        int tokenCount = tokenizeLine(line, tokens);

        if (tokenCount == 0) continue;

        // Check if this line starts with data type tokens for declaration
        int dataTypeTokensLen = 0;
        char dataTypeBuffer[100] = "";
        int i;
        bool isFunctionDecl = false;
        for (i = 0; i < tokenCount; i++) {
            if (isDataTypeToken(tokens[i])) {
                if (dataTypeTokensLen > 0) strcat(dataTypeBuffer, " ");
                strcat(dataTypeBuffer, tokens[i]);
                dataTypeTokensLen++;
            } else if (dataTypeTokensLen > 0 && i + 1 < tokenCount && strcmp(tokens[i+1], "(") == 0) {
                // Function declaration detected
                isFunctionDecl = true;
                break;
            } else {
                break;
            }
        }

        // Process declarations (variables or functions)
        if (dataTypeTokensLen > 0 && dataTypeTokensLen < tokenCount) {
            if (isFunctionDecl) {
                // Handle function declaration
                if (isValidIdentifier_Advanced(tokens[i])) {
                    int found = 0;
                    for (int k = 0; k < validIdentifiersCount; k++) {
                        if (strcmp(validIdentifiers[k], tokens[i]) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        strcpy(validIdentifiers[validIdentifiersCount++], tokens[i]);
                    }
                    addToSymbolTable(dataTypeBuffer, tokens[i], "-", lineno);
                } else {
                    int found = 0;
                    for (int k = 0; k < invalidIdentifiersCount; k++) {
                        if (strcmp(invalidIdentifiers[k], tokens[i]) == 0) {
                            found = 1;
                            break;
                        }
                    }
                    if (!found) {
                        strcpy(invalidIdentifiers[invalidIdentifiersCount++], tokens[i]);
                    }
                }
            } else {
                // Handle variable declarations
                processDeclarationTokens(tokens, dataTypeTokensLen, tokenCount, dataTypeBuffer, lineno);
            }
        }

        // Process all tokens for token categories
        for (int t = 0; t < tokenCount; t++) {
            char *token = tokens[t];

            // Skip empty tokens
            if (strlen(token) == 0) continue;

            // Keywords
            if (isKeyword(token)) {
                int found = 0;
                for (int k = 0; k < keywordsCount; k++) {
                    if (strcmp(keywordsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found && strlen(token) < 100) {
                    strcpy(keywordsFound[keywordsCount++], token);
                }
                continue;
            }
            // Multi-char operators
            if (isMultiCharOp(token)) {
                int found = 0;
                for (int k = 0; k < multiCharOpsCount; k++) {
                    if (strcmp(multiCharOpsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(multiCharOpsFound[multiCharOpsCount++], token);
                }
                continue;
            }
            // Single char operators
            if (isOperatorString(token)) {
                int found = 0;
                for (int k = 0; k < operatorsCount; k++) {
                    if (strcmp(operatorsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(operatorsFound[operatorsCount++], token);
                }
                continue;
            }
            // Separators
            if (strlen(token) == 1 && isSeparator(token[0])) {
                int found = 0;
                for (int k = 0; k < separatorsCount; k++) {
                    if (strcmp(separatorsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(separatorsFound[separatorsCount++], token);
                }
                continue;
            }
            // Brackets
            if (strlen(token) == 1 && isBracket(token[0])) {
                int found = 0;
                for (int k = 0; k < bracketsCount; k++) {
                    if (strcmp(bracketsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(bracketsFound[bracketsCount++], token);
                }
                continue;
            }
            // Special symbols
            if (strlen(token) == 1 && isSpecialSymbol(token[0])) {
                int found = 0;
                for (int k = 0; k < specialSymbolsCount; k++) {
                    if (strcmp(specialSymbolsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(specialSymbolsFound[specialSymbolsCount++], token);
                }
                continue;
            }
            // String literals
            if (token[0] == '"' && token[strlen(token)-1] == '"') {
                int found = 0;
                for (int k = 0; k < stringLiteralsCount; k++) {
                    if (strcmp(stringLiteralsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(stringLiteralsFound[stringLiteralsCount++], token);
                }
                continue;
            }
            // Character literals
            if (token[0] == '\'' && token[strlen(token)-1] == '\'') {
                int found = 0;
                for (int k = 0; k < stringLiteralsCount; k++) {
                    if (strcmp(stringLiteralsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(stringLiteralsFound[stringLiteralsCount++], token);
                }
                continue;
            }
            // Numeric literals
            if (isdigit(token[0])) {
                int found = 0;
                for (int k = 0; k < numericsCount; k++) {
                    if (strcmp(numericsFound[k], token) == 0) {
                        found = 1;
                        break;
                    }
                }
                if (!found) {
                    strcpy(numericsFound[numericsCount++], token);
                }
                continue;
            }
            // Identifiers are only checked in declaration contexts
            // Tokens not in any category
            int found = 0;
            for (int k = 0; k < othersCount; k++) {
                if (strcmp(othersFound[k], token) == 0) {
                    found = 1;
                    break;
                }
            }
            if (!found) {
                strcpy(othersFound[othersCount++], token);
            }
        }
    }

    // Print to output.txt
    fprintf(output, "***************************************************\n");
    fprintf(output, "*          LEXICAL ANALYSIS REPORT                 *\n");
    fprintf(output, "*         Tourist Management System Code           *\n");
    fprintf(output, "***************************************************\n\n");

    // Print Valid and Invalid Identifiers
    fprintf(output, "Valid Variables/Identifiers (Count: %d): [", validIdentifiersCount);
    for (int i = 0; i < validIdentifiersCount; i++) {
        fprintf(output, "%s%s", validIdentifiers[i], (i == validIdentifiersCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    fprintf(output, "Invalid Variables/Identifiers (Count: %d): [", invalidIdentifiersCount);
    for (int i = 0; i < invalidIdentifiersCount; i++) {
        fprintf(output, "%s%s", invalidIdentifiers[i], (i == invalidIdentifiersCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Print all tokens by category
    fprintf(output, "=========== TOKENS BY CATEGORY ===========\n\n");

    // Print Keywords
    fprintf(output, "Keywords: [");
    for (int i = 0; i < keywordsCount; i++) {
        fprintf(output, "%s%s", keywordsFound[i], (i == keywordsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Print Identifiers (valid ones)
    fprintf(output, "Identifiers: [");
    for (int i = 0; i < validIdentifiersCount; i++) {
        fprintf(output, "%s%s", validIdentifiers[i], (i == validIdentifiersCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Print Numeric literals
    fprintf(output, "Numeric: [");
    for (int i = 0; i < numericsCount; i++) {
        fprintf(output, "%s%s", numericsFound[i], (i == numericsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // String literals
    fprintf(output, "String Literals: [");
    for (int i = 0; i < stringLiteralsCount; i++) {
        fprintf(output, "%s%s", stringLiteralsFound[i], (i == stringLiteralsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Multi-char operators
    fprintf(output, "Multi-char Operators: [");
    for (int i = 0; i < multiCharOpsCount; i++) {
        fprintf(output, "%s%s", multiCharOpsFound[i], (i == multiCharOpsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Single char operators
    fprintf(output, "Operators: [");
    for (int i = 0; i < operatorsCount; i++) {
        fprintf(output, "%s%s", operatorsFound[i], (i == operatorsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Separators
    fprintf(output, "Separators: [");
    for (int i = 0; i < separatorsCount; i++) {
        fprintf(output, "%s%s", separatorsFound[i], (i == separatorsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Brackets
    fprintf(output, "Brackets: [");
    for (int i = 0; i < bracketsCount; i++) {
        fprintf(output, "%s%s", bracketsFound[i], (i == bracketsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Special Symbols
    fprintf(output, "Special Symbols: [");
    for (int i = 0; i < specialSymbolsCount; i++) {
        fprintf(output, "%s%s", specialSymbolsFound[i], (i == specialSymbolsCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Other tokens
    fprintf(output, "Others: [");
    for (int i = 0; i < othersCount; i++) {
        fprintf(output, "%s%s", othersFound[i], (i == othersCount -1) ? "" : ", ");
    }
    fprintf(output, "]\n\n");

    // Print Symbol Table
    fprintf(output, "=========== SYMBOL TABLE ===========\n");
    fprintf(output, "---------------------------------------------------------------\n");
    fprintf(output, "| Name            | DataType               | Value          | Line |\n");
    fprintf(output, "---------------------------------------------------------------\n");
    for (int i = 0; i < symbolCount; i++) {
        fprintf(output, "| %-15s | %-21s | %-14s | %-4d |\n",
                symbolTable[i].name, symbolTable[i].type, symbolTable[i].value, symbolTable[i].line);
    }
    fprintf(output, "---------------------------------------------------------------\n");

    fprintf(output, "\n***************************************************\n");
    fprintf(output, "*                 END OF REPORT                    *\n");
    fprintf(output, "***************************************************\n");
}

// Interactive validator for terminal input
void interactiveValidator() {
    char input[100];
    printf("\n========================================\n");
    printf("Variable Declaration Validity Check  \n");
    printf("========================================\n");

    while (1) {
        printf("\nDo you want to check a variable name? (Y/N): ");
        if (!fgets(input, sizeof(input), stdin)) break;
        if (input[0] == 'N' || input[0] == 'n') {
            printf("Exiting validation mode.\n");
            break;
        } else if (input[0] == 'Y' || input[0] == 'y') {
            while (1) {
                printf("\nEnter variable/identifier name to validate (or N to exit): ");
                if (!fgets(input, sizeof(input), stdin)) return;
                // Remove newline
                input[strcspn(input, "\n")] = 0;

                if (input[0] == 'N' || input[0] == 'n') {
                    printf("Exiting validation mode.\n");
                    return;
                }

                printf("\nChecking variable: \"%s\"\n", input);
                // Validate using your advanced pattern:
                if (isValidIdentifier_Advanced(input)) {
                    printf("Valid identifier!\n");
                    printf("Reason: \n");
                    int i = 0;
                    if (input[i] == '#' || input[i] == '@' || input[i] == '!') {
                        printf("  - Optional leading character (#, @, !): Present (%c)\n", input[i]);
                        i++;
                    } else {
                        printf("  - Optional leading character (#, @, !): Not present\n");
                    }
                    int letterCount = 0;
                    char prev_letter = '\0';
                    int consec_letter = 0;
                    bool letter_consec_ok = true;
                    while (i < (int)strlen(input) && islower(input[i])) {
                        letterCount++;
                        if (letterCount == 1) {
                            prev_letter = input[i];
                            consec_letter = 1;
                        } else {
                            if (input[i] == prev_letter) {
                                consec_letter++;
                                if (consec_letter > 2) letter_consec_ok = false;
                            } else {
                                consec_letter = 1;
                                prev_letter = input[i];
                            }
                        }
                        i++;
                    }
                    printf("  - Lowercase letters (a-z) count: %d (required 4-7)\n", letterCount);
                    printf("  - No more than two consecutive same letters: %s\n", letter_consec_ok ? "Yes" : "No");
                    int digitCount = 0;
                    char prev_digit = '\0';
                    int consec_digit = 0;
                    bool digit_consec_ok = true;
                    while (i < (int)strlen(input) && isdigit(input[i])) {
                        digitCount++;
                        if (digitCount == 1) {
                            prev_digit = input[i];
                            consec_digit = 1;
                        } else {
                            if (input[i] == prev_digit) {
                                consec_digit++;
                                if (consec_digit > 2) digit_consec_ok = false;
                            } else {
                                consec_digit = 1;
                                prev_digit = input[i];
                            }
                        }
                        i++;
                    }
                    printf("  - Digits (0-9) count: %d (required 2-4)\n", digitCount);
                    printf("  - No more than two consecutive same digits: %s\n", digit_consec_ok ? "Yes" : "No");
                    printf("  - Ends with \"@r\": Yes\n");
                } else {
                    printf("Invalid identifier!\n");
                    printf("Reason:\n");
                    int i = 0;
                    bool has_prefix = false;
                    if (input[i] == '#' || input[i] == '@' || input[i] == '!') {
                        has_prefix = true;
                        i++;
                    }
                    int letterCount = 0;
                    char prev_letter = '\0';
                    int consec_letter = 0;
                    bool letter_consec_ok = true;
                    while (i < (int)strlen(input) && islower(input[i])) {
                        letterCount++;
                        if (letterCount == 1) {
                            prev_letter = input[i];
                            consec_letter = 1;
                        } else {
                            if (input[i] == prev_letter) {
                                consec_letter++;
                                if (consec_letter > 2) letter_consec_ok = false;
                            } else {
                                consec_letter = 1;
                                prev_letter = input[i];
                            }
                        }
                        i++;
                    }
                    int digitCount = 0;
                    char prev_digit = '\0';
                    int consec_digit = 0;
                    bool digit_consec_ok = true;
                    while (i < (int)strlen(input) && isdigit(input[i])) {
                        digitCount++;
                        if (digitCount == 1) {
                            prev_digit = input[i];
                            consec_digit = 1;
                        } else {
                            if (input[i] == prev_digit) {
                                consec_digit++;
                                if (consec_digit > 2) digit_consec_ok = false;
                            } else {
                                consec_digit = 1;
                                prev_digit = input[i];
                            }
                        }
                        i++;
                    }
                    if (letterCount < 4 || letterCount > 7) {
                        printf("  - Lowercase letters count not in 4 to 7 (found %d)\n", letterCount);
                    }
                    if (!letter_consec_ok) {
                        printf("  - More than two consecutive same letters found\n");
                    }
                    if (digitCount < 2 || digitCount > 4) {
                        printf("  - Digits count not in 2 to 4 (found %d)\n", digitCount);
                    }
                    if (!digit_consec_ok) {
                        printf("  - More than two consecutive same digits found\n");
                    }
                    if (i + 1 >= (int)strlen(input) || strncmp(&input[i], "@r", 2) != 0) {
                        printf("  - Does not end with \"@r\"\n");
                    }
                    if (!has_prefix && !(islower(input[0]))) {
                        printf("  - Must start with optional '#', '@', '!' followed by lowercase letters\n");
                    }
                }
            }
        } else {
            printf("Invalid choice, please type Y or N.\n");
        }
    }
}

// ==================== Main ====================
int main() {
    FILE *input = fopen("input.txt", "r");
    if (!input) {
        printf("Error: Could not open input.txt\n");
        return 1;
    }

    FILE *output = fopen("output.txt", "w");
    if (!output) {
        printf("Error: Could not open output.txt\n");
        fclose(input);
        return 1;
    }

    processFile(input, output);

    fclose(input);
    fclose(output);

    if (invalidIdentifiersCount > 0) {
        printf("Invalid identifiers found in input.txt. Please remove or correct them to make the code valid.\n");
    }

    printf("\n==============================\n");
    printf("Lexical analysis completed.\n");
    printf("See 'output.txt' for detailed token categories and symbol table.\n");
    printf("==============================\n");

    interactiveValidator();

    return 0;
}
