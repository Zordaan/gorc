#pragma once

#include "ast/node.hpp"
#include <string>

namespace gorc {

    /* General */

    class simple_word;
    class variable_name;

    using word = variant<simple_word*,
                         variable_name*>;

    class simple_word : public visitable_ast_node<simple_word> {
    public:
        std::string value;

        simple_word(diagnostic_context_location const &loc,
                    std::string const &value);
    };

    class variable_name : public visitable_ast_node<variable_name> {
    public:
        std::string name;

        variable_name(diagnostic_context_location const &loc,
                      std::string const &name);
    };

    class argument : public visitable_ast_node<argument> {
    public:
        ast_list_node<word*> *words;

        argument(diagnostic_context_location const &loc,
                 ast_list_node<word*> *words);
    };

    /* Commands */

    class subcommand : public visitable_ast_node<subcommand> {
    public:
        ast_list_node<argument*> *arguments;

        subcommand(diagnostic_context_location const &loc,
                   ast_list_node<argument*> *arguments);
    };

    class pipe_command;
    using command = variant<pipe_command*>;

    class pipe_command : public visitable_ast_node<pipe_command> {
    public:
        ast_list_node<subcommand*> *subcommands;

        pipe_command(diagnostic_context_location const &loc,
                     ast_list_node<subcommand*> *subcommands);
    };

    /* Statements */

    class command_statement;
    class assignment_statement;
    class export_statement;
    using statement = variant<command_statement*,
                              assignment_statement*,
                              export_statement*>;

    class command_statement : public visitable_ast_node<command_statement> {
    public:
        command *cmd;

        command_statement(diagnostic_context_location const &loc,
                          command *cmd);
    };

    class assignment_statement : public visitable_ast_node<assignment_statement> {
    public:
        variable_name *var;
        argument *value;

        assignment_statement(diagnostic_context_location const &loc,
                             variable_name *var,
                             argument *value);
    };

    class export_statement : public visitable_ast_node<export_statement> {
    public:
        variable_name *var;

        export_statement(diagnostic_context_location const &loc,
                         variable_name *var);
    };

    class translation_unit : public visitable_ast_node<translation_unit> {
    public:
        ast_list_node<statement*> *code;

        translation_unit(diagnostic_context_location const &loc,
                         ast_list_node<statement*> *code);
    };

}
