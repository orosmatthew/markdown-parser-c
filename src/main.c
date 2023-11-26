#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_SIZE 1024
#define MAX_CAPTURE_SIZE 1024

typedef enum {
    MD_TYPE_HEADING_1,
    MD_TYPE_HEADING_2,
    MD_TYPE_HEADING_3,
    MD_TYPE_TEXT_BOLD,
    MD_TYPE_TEXT_ITALICS,
    MD_TYPE_BLOCK_QUOTE,
    MD_TYPE_TEXT,
    MD_TYPE_LINE
} MdNodeType;

typedef struct MdNode {
    MdNodeType type;
    char content[MAX_LINE_SIZE];
    struct MdNode* next;
} MdNode;

void md_list_append(MdNode** head, const MdNodeType type, const char* content)
{
    MdNode* new_node = malloc(sizeof(MdNode));
    new_node->type = type;
    strncpy(new_node->content, content, sizeof(new_node->content));
    new_node->next = NULL;

    if (*head == NULL) {
        *head = new_node;
    }
    else {
        MdNode* node = *head;
        while (node->next != NULL) {
            node = node->next;
        }
        node->next = new_node;
    }
}

bool match_no_capture(const regex_t regex, const char* content)
{
    return regexec(&regex, content, 0, NULL, 0) == 0;
}

bool match_single_capture(const regex_t regex, const char* content, char* capture, const size_t capture_size)
{
    regmatch_t matches[2]; // one for whole match, second for capture group
    if (regexec(&regex, content, 2, matches, 0) == 0) {
        if (matches[1].rm_so != -1 && matches[1].rm_eo != -1) {
            const int capture_len = matches[1].rm_eo - matches[1].rm_so;
            if (capture_len > 0 && capture_len < capture_size - 1) {
                strncpy(capture, content + matches[1].rm_so, capture_len);
                capture[capture_len] = '\0';
                return true;
            }
        }
    }
    return false;
}

typedef struct {
    regex_t heading_1;
    regex_t heading_2;
    regex_t heading_3;
    regex_t text_bold;
    regex_t text_italics;
    regex_t block_quote;
    regex_t line;
} MdRegexes;

MdRegexes md_regexes_create()
{
    MdRegexes r;
    regcomp(&r.heading_1, "# +(.*)", REG_EXTENDED);
    regcomp(&r.heading_2, "## +(.*)", REG_EXTENDED);
    regcomp(&r.heading_3, "### +(.*)", REG_EXTENDED);
    regcomp(&r.text_bold, "\\*\\*(.*?)\\*\\*", REG_EXTENDED);
    regcomp(&r.text_italics, "\\*(.*?)\\*", REG_EXTENDED);
    regcomp(&r.block_quote, "> *(.*)", REG_EXTENDED);
    regcomp(&r.line, " *- *- *-.*", REG_EXTENDED);
    return r;
}

void match_md(const MdRegexes* r, const char* content, MdNode** md_list)
{
    char capture[MAX_CAPTURE_SIZE];
    if (match_single_capture(r->heading_3, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_HEADING_3, capture);
    }
    else if (match_single_capture(r->heading_2, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_HEADING_2, capture);
    }
    else if (match_single_capture(r->heading_1, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_HEADING_1, capture);
    }
    else if (match_single_capture(r->block_quote, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_BLOCK_QUOTE, capture);
    }
    else if (match_no_capture(r->line, content)) {
        md_list_append(md_list, MD_TYPE_LINE, "");
    }
    else if (match_single_capture(r->text_bold, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_TEXT_BOLD, capture);
    }
    else if (match_single_capture(r->text_italics, content, capture, sizeof(capture))) {
        md_list_append(md_list, MD_TYPE_TEXT_ITALICS, capture);
    }
    else {
        md_list_append(md_list, MD_TYPE_TEXT, content);
    }
}

void md_node_to_html(const MdNode* node, char* result, const size_t result_size)
{
    switch (node->type) {
    case MD_TYPE_HEADING_1:
        snprintf(result, result_size, "<h1>%s</h1>\n", node->content);
        return;
    case MD_TYPE_HEADING_2:
        snprintf(result, result_size, "<h2>%s</h2>\n", node->content);
        return;
    case MD_TYPE_HEADING_3:
        snprintf(result, result_size, "<h3>%s</h3>\n", node->content);
        return;
    case MD_TYPE_TEXT_BOLD:
        snprintf(result, result_size, "<b>%s</b><br />\n", node->content);
        return;
    case MD_TYPE_TEXT_ITALICS:
        snprintf(result, result_size, "<i>%s</i><br />\n", node->content);
        return;
    case MD_TYPE_BLOCK_QUOTE:
        snprintf(result, result_size, "<blockquote>%s</blockquote>\n", node->content);
        return;
    case MD_TYPE_TEXT:
        snprintf(result, result_size, "%s<br />\n", node->content);
        return;
    case MD_TYPE_LINE:
        snprintf(result, result_size, "<hr />\n");
    }
}

void write_md_to_html(FILE* out, const MdNode* md_list)
{
    const MdNode* node = md_list;
    while (node != NULL) {
        char buffer[MAX_LINE_SIZE];
        md_node_to_html(node, buffer, sizeof(buffer));
        fputs(buffer, out);
        node = node->next;
    }
}

int main()
{
    FILE* in_file = fopen("../test.md", "r");
    if (in_file == NULL) {
        perror("Error opening file");
    }

    const MdRegexes regexes = md_regexes_create();

    MdNode* md_list = NULL;
    char line_buffer[MAX_LINE_SIZE];
    while (fgets(line_buffer, MAX_LINE_SIZE, in_file) != NULL) {
        line_buffer[strcspn(line_buffer, "\r\n")] = '\0';
        if (strcmp(line_buffer, "") != 0) {
            match_md(&regexes, line_buffer, &md_list);
        }
    }
    fclose(in_file);

    FILE* out_file = fopen("../test.html", "w");
    if (out_file == NULL) {
        perror("Error creating file");
    }
    write_md_to_html(out_file, md_list);
    fclose(out_file);

    return EXIT_SUCCESS;
}
