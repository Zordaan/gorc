#pragma once

#include "cog/ast/ast.hpp"
#include "cog/ast/factory.hpp"
#include "cog/script/script.hpp"
#include "cog/script/constant_table.hpp"
#include "cog/script/verb_table.hpp"
#include "expr_val.hpp"

namespace gorc {
    namespace cog {

        class rvalue_visitor {
        private:
            script &out_script;
            ast::factory &factory;
            constant_table const &constants;
            verb_table const &verbs;
            bool result_is_used;

        public:
            rvalue_visitor(script &out_script,
                           ast::factory &factory,
                           constant_table const &constants,
                           verb_table const &verbs,
                           bool result_is_used);

            expr_val visit(ast::immediate_expression &);
            expr_val visit(ast::string_literal_expression &);
            expr_val visit(ast::integer_literal_expression &);
            expr_val visit(ast::float_literal_expression &);
            expr_val visit(ast::vector_literal_expression &);
            expr_val visit(ast::identifier_expression &);
            expr_val visit(ast::subscript_expression &);
            expr_val visit(ast::method_call_expression &);
            expr_val visit(ast::unary_expression &);
            expr_val visit(ast::infix_expression &);
            expr_val visit(ast::assignment_expression &);
            expr_val visit(ast::comma_expression &);
        };

        void visit_and_fold_boolean_condition(ast::expression &e,
                                              script &out_script,
                                              ast::factory &factory,
                                              constant_table const &constants,
                                              verb_table const &verbs);

        void visit_and_fold_array_subscript(ast::expression &e,
                                            script &out_script,
                                            ast::factory &factory,
                                            constant_table const &constants,
                                            verb_table const &verbs);
    }
}