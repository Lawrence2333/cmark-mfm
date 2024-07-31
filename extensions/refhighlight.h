#ifndef CMARK_GFM_REFHIGHLIGHT_H
#define CMARK_GFM_REFHIGHLIGHT_H

#include "cmark-gfm-core-extensions.h"

extern cmark_node_type CMARK_NODE_REFHIGHLIGHT;
cmark_syntax_extension *create_refhighlight_extension(void);

#endif
