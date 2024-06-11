#include <stdbool.h>

#include "ragtag.h"
#include <parser.h>
#include <render.h>

cmark_node_type CMARK_NODE_RAGTAG;

static cmark_node *match(cmark_syntax_extension *self, cmark_parser *parser,
                         cmark_node *parent, unsigned char character,
                         cmark_inline_parser *inline_parser) {
  cmark_node *ragtag;
  int column = cmark_inline_parser_get_column(inline_parser);
  int offset = cmark_inline_parser_get_offset(inline_parser);
  unsigned char cur_char = cmark_inline_parser_peek_at(inline_parser, offset);
  char buffer[31]; // 30 digits

  // hack, starts from ^
  if (cur_char != '^') {
    return NULL;
  }

  // then we see if there's a [ before ^
  unsigned char before_char = cmark_inline_parser_peek_at(inline_parser, offset - 1);
  if (before_char != '[') {
    return NULL;
  }

  // start finding the closing ^]
  int seeker = offset + 1;
  while (seeker <= offset + 30 && cmark_inline_parser_peek_at(inline_parser, seeker) != '^') {
    unsigned char c = cmark_inline_parser_peek_at(inline_parser, seeker);
    if (c < '0' || c > '9') { // numbers only
      cmark_inline_parser_set_offset(inline_parser, offset);
      return NULL;
    }
    int index = seeker - offset - 1;
    memset(buffer + index, c, 1);
    seeker++;
  }
  int len_of_content = seeker - offset - 1;
  buffer[len_of_content] = 0; // end of buffer

  // found ^, then ]
  unsigned char after_char = cmark_inline_parser_peek_at(inline_parser, seeker + 1);
  if (after_char != ']') {
    return NULL;
  }

  // found everything,
  // before we create the node, we should pop the opening [ bracket
  if (cmark_inline_parser_in_bracket(inline_parser, 0)
     || cmark_inline_parser_in_bracket(inline_parser, 1)
     || cmark_inline_parser_in_bracket(inline_parser, 2)) {
    cmark_inline_parser_free_last_bracket(inline_parser);
    cmark_inline_parser_pop_bracket(inline_parser);
  }

  // advance to the character after ]
  cmark_inline_parser_set_offset(inline_parser, seeker + 2);
  // create the text node within
  cmark_node *text = cmark_node_new_with_mem(CMARK_NODE_TEXT, parser->mem);
  // .. with all text between [^ and ^]
  cmark_node_set_string_content(text, buffer);
  cmark_node_set_literal(text, buffer);
  text->start_line = text->end_line = cmark_inline_parser_get_line(inline_parser);
  text->start_column = column + 1;
  text->end_column = text->start_column + len_of_content;

  // create the ragtag node
  ragtag = cmark_node_new_with_mem(CMARK_NODE_RAGTAG, parser->mem);
  if (!cmark_node_set_type(ragtag, CMARK_NODE_RAGTAG)) {
    return NULL;
  }
  cmark_node_set_syntax_extension(ragtag, self);
  ragtag->start_line = ragtag->end_line = cmark_inline_parser_get_line(inline_parser);
  int len_of_tag = len_of_content + 4/*[^^]*/;
  ragtag->start_column = column - 1;
  ragtag->end_column = ragtag->start_column + len_of_tag;

  cmark_node_append_child(ragtag, text);

  return ragtag;
}

static const char *get_type_string(cmark_syntax_extension *extension,
                                   cmark_node *node) {
  return node->type == CMARK_NODE_RAGTAG ? "ragtag" : "<unknown>";
}

static int can_contain(cmark_syntax_extension *extension, cmark_node *node,
                       cmark_node_type child_type) {
  if (node->type != CMARK_NODE_RAGTAG)
    return false;

  return CMARK_NODE_TYPE_INLINE_P(child_type);
}

static void commonmark_render(cmark_syntax_extension *extension,
                              cmark_renderer *renderer, cmark_node *node,
                              cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  if (entering) {
    renderer->out(renderer, node, "[^", false, LITERAL);
  } else {
    renderer->out(renderer, node, "^]", false, LITERAL);
  }
}

static void html_render(cmark_syntax_extension *extension,
                        cmark_html_renderer *renderer, cmark_node *node,
                        cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  if (entering) {
    cmark_strbuf_puts(renderer->html, "<span class=\"ragtag\">");
  } else {
    cmark_strbuf_puts(renderer->html, "</span>");
  }
}

static void plaintext_render(cmark_syntax_extension *extension,
                             cmark_renderer *renderer, cmark_node *node,
                             cmark_event_type ev_type, int options) {
  bool entering = (ev_type == CMARK_EVENT_ENTER);
  if (entering) {
    renderer->out(renderer, node, "[^", false, LITERAL);
  } else {
    renderer->out(renderer, node, "^]", false, LITERAL);
  }
}

cmark_syntax_extension *create_ragtag_extension(void) {
  cmark_syntax_extension *ext = cmark_syntax_extension_new("ragtag");

  cmark_syntax_extension_set_get_type_string_func(ext, get_type_string);
  cmark_syntax_extension_set_can_contain_func(ext, can_contain);

  cmark_syntax_extension_set_commonmark_render_func(ext, commonmark_render);
  cmark_syntax_extension_set_html_render_func(ext, html_render);
  cmark_syntax_extension_set_plaintext_render_func(ext, plaintext_render);

  CMARK_NODE_RAGTAG = cmark_syntax_extension_add_node(1);

  cmark_syntax_extension_set_match_inline_func(ext, match);

  return ext;
}
