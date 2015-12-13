#pragma once

#include "ast.hpp"
#include "sexpr/sexpr.hpp"

namespace gorc {

    class expression_visitor {
    public:
        sexpr visit(argument_expression &) const;
        sexpr visit(unary_expression &) const;
        sexpr visit(infix_expression &) const;
        sexpr visit(nil_expression &) const;
        sexpr visit(call_expression &) const;
    };

}