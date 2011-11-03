//
//  Copyright (C) 2011  Nick Gasson
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef _TREE_H
#define _TREE_H

#include "lib.h"
#include "ident.h"
#include "type.h"
#include "loc.h"

#include <stdint.h>

typedef enum port_mode {
   PORT_INVALID,
   PORT_IN,
   PORT_OUT,
   PORT_INOUT,
   PORT_BUFFER
} port_mode_t;

typedef enum tree_kind {
   T_ENTITY,
   T_ARCH,
   T_PORT_DECL,
   T_FCALL,
   T_LITERAL,
   T_SIGNAL_DECL,
   T_VAR_DECL,
   T_PROCESS,
   T_REF,
   T_WAIT,
   T_TYPE_DECL,
   T_VAR_ASSIGN,
   T_PACKAGE,
   T_SIGNAL_ASSIGN,
   T_QUALIFIED,
   T_ENUM_LIT,
   T_CONST_DECL,
   T_FUNC_DECL,
   T_ELAB,
   T_AGGREGATE,
   T_ASSERT,
   T_ATTR_REF,
   T_ARRAY_REF,
   T_ARRAY_SLICE,
   T_INSTANCE,
   T_IF,

   T_LAST_TREE_KIND
} tree_kind_t;

typedef struct tree *tree_t;

typedef struct literal {
   union {
      int64_t i;
      double  r;
   };
   enum { L_INT, L_REAL } kind;
} literal_t;

typedef struct assoc {
   union {
      unsigned pos;
      tree_t   name;
      range_t  range;
   };
   tree_t value;
   enum { A_POS, A_NAMED, A_RANGE, A_OTHERS } kind;
} assoc_t;

typedef struct param {
   tree_t value;
   union {
      range_t  range;   // P_RANGE
      ident_t  name;    // P_NAME
      unsigned pos;     // P_POS
   };
   enum { P_POS, P_NAMED, P_RANGE } kind;
} param_t;

typedef struct tree_wr_ctx *tree_wr_ctx_t;
typedef struct tree_rd_ctx *tree_rd_ctx_t;

tree_t tree_new(tree_kind_t kind);
tree_kind_t tree_kind(tree_t t);
void tree_change_kind(tree_t t, tree_kind_t kind);

const loc_t *tree_loc(tree_t t);
void tree_set_loc(tree_t t, const loc_t *loc);

// T_PORT_DECL, T_SIGNAL_DECL, T_VAR_DECL, T_REF, T_TYPE_DECL,
// T_CONST_DECL, T_FUNC_DECL
type_t tree_type(tree_t t);
void tree_set_type(tree_t t, type_t ty);

// T_ENTITY, T_PORT_DECL, T_FCALL, T_ARCH, T_SIGNAL_DECL, T_PROCESS,
// T_VAR_DECL, T_REF, T_TYPE_DECL, T_PACKAGE, T_QUALIFIED, T_ENUM_LIT,
// T_CONST_DECL, T_FUNC_DECL, T_ATTR_REF, T_INSTANCE
ident_t tree_ident(tree_t t);
void tree_set_ident(tree_t t, ident_t i);
bool tree_has_ident(tree_t t);

// T_ARCH, T_ATTR_REF, T_INSTANCE
ident_t tree_ident2(tree_t t);
void tree_set_ident2(tree_t t, ident_t i);

// T_ENTITY, T_FUNC_DECL
unsigned tree_ports(tree_t t);
tree_t tree_port(tree_t t, unsigned n);
void tree_add_port(tree_t t, tree_t d);

// T_PORT_DECL
port_mode_t tree_port_mode(tree_t t);
void tree_set_port_mode(tree_t t, port_mode_t mode);

// T_ENTITY
unsigned tree_generics(tree_t t);
tree_t tree_generic(tree_t t, unsigned n);
void tree_add_generic(tree_t t, tree_t d);

// T_INSTANCE
unsigned tree_genmaps(tree_t t);
param_t tree_genmap(tree_t t, unsigned n);
void tree_add_genmap(tree_t t, param_t e);
void tree_change_genmap(tree_t t, unsigned n, param_t e);

// T_FCALL, T_ARRAY_REF, T_INSTANCE
unsigned tree_params(tree_t t);
param_t tree_param(tree_t t, unsigned n);
void tree_add_param(tree_t t, param_t e);
void tree_change_param(tree_t t, unsigned n, param_t e);

// T_LITERAL
literal_t tree_literal(tree_t t);
void tree_set_literal(tree_t t, literal_t lit);

// T_PORT_DECL, T_SIGNAL_DECL, T_VAR_DECL, T_VAR_ASSIGN,
// T_SIGNAL_ASSIGN, T_QUALIFIED, T_CONST_DECL, T_ASSERT,
// T_ARRAY_REF, T_IF
bool tree_has_value(tree_t t);
tree_t tree_value(tree_t t);
void tree_set_value(tree_t t, tree_t v);

// T_ARCH, T_PROCESS, T_PACKAGE
unsigned tree_decls(tree_t t);
tree_t tree_decl(tree_t t, unsigned n);
void tree_add_decl(tree_t t, tree_t d);

// T_ARCH, T_PROCESS, T_IF
unsigned tree_stmts(tree_t t);
tree_t tree_stmt(tree_t t, unsigned n);
void tree_add_stmt(tree_t t, tree_t d);
void tree_change_stmt(tree_t t, unsigned n, tree_t d);

// T_WAIT
bool tree_has_delay(tree_t t);
tree_t tree_delay(tree_t t);
void tree_set_delay(tree_t t, tree_t d);

// T_WAIT, T_PROCESS
unsigned tree_triggers(tree_t t);
tree_t tree_trigger(tree_t t, unsigned n);
void tree_add_trigger(tree_t t, tree_t s);
void tree_change_trigger(tree_t t, unsigned n, tree_t s);

// T_VAR_ASSIGN, T_SIGNAL_ASSIGN
tree_t tree_target(tree_t t);
void tree_set_target(tree_t t, tree_t lhs);

// T_REF, T_FCALL, T_INSTANCE
tree_t tree_ref(tree_t t);
void tree_set_ref(tree_t t, tree_t decl);

// T_ENTITY, T_ARCH, T_PACKAGE
unsigned tree_contexts(tree_t t);
ident_t tree_context(tree_t t, unsigned n);
void tree_add_context(tree_t t, ident_t ctx);

// T_AGGREGATE
unsigned tree_assocs(tree_t t);
assoc_t tree_assoc(tree_t t, unsigned n);
void tree_add_assoc(tree_t t, assoc_t a);
void tree_change_assoc(tree_t t, unsigned n, assoc_t a);

// T_ASSERT
tree_t tree_severity(tree_t t);
void tree_set_severity(tree_t t, tree_t s);

// T_ASSERT
tree_t tree_message(tree_t t);
void tree_set_message(tree_t t, tree_t m);

// T_ENUM_LIT
unsigned tree_pos(tree_t t);
void tree_set_pos(tree_t t, unsigned pos);

// T_SIGNAL_DECL
unsigned tree_drivers(tree_t t);
tree_t tree_driver(tree_t t, unsigned n);
void tree_add_driver(tree_t t, tree_t d);

// T_SIGNAL_DECL
unsigned tree_sub_drivers(tree_t t, unsigned elem);
tree_t tree_sub_driver(tree_t t, unsigned elem, unsigned n);
void tree_add_sub_driver(tree_t t, unsigned elem, tree_t p);

// T_ARRAY_SLICE
range_t tree_range(tree_t t);
void tree_set_range(tree_t t, range_t r);

void tree_add_attr_str(tree_t t, ident_t name, const char *str);
const char *tree_attr_str(tree_t t, ident_t name);
void tree_add_attr_int(tree_t t, ident_t name, int n);
int tree_attr_int(tree_t t, ident_t name, int def);
void tree_add_attr_ptr(tree_t t, ident_t name, void *ptr);
void *tree_attr_ptr(tree_t t, ident_t name);
tree_t tree_attr_tree(tree_t t, ident_t name);
void tree_add_attr_tree(tree_t t, ident_t name, tree_t val);

// Utility functions
int64_t assume_int(tree_t t);
void range_bounds(range_t r, int64_t *low, int64_t *high);

typedef void (*tree_visit_fn_t)(tree_t t, void *context);
unsigned tree_visit(tree_t t, tree_visit_fn_t fn, void *context);
unsigned tree_visit_only(tree_t t, tree_visit_fn_t fn,
                         void *context, tree_kind_t kind);

typedef tree_t (*tree_rewrite_fn_t)(tree_t t, void *context);
tree_t tree_rewrite(tree_t t, tree_rewrite_fn_t fn, void *context);

void tree_gc(void);

tree_wr_ctx_t tree_write_begin(FILE *f);
void tree_write(tree_t t, tree_wr_ctx_t ctx);
void tree_write_end(tree_wr_ctx_t ctx);
FILE *tree_write_file(tree_wr_ctx_t ctx);

tree_rd_ctx_t tree_read_begin(FILE *f);
tree_t tree_read(tree_rd_ctx_t ctx);
void tree_read_end(tree_rd_ctx_t ctx);
FILE *tree_read_file(tree_rd_ctx_t ctx);

#endif  // _TREE_H
