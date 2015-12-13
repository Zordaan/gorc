#include "program_visitor.hpp"
#include "log/log.hpp"
#include "stack.hpp"
#include "sexpr/sexpr_helpers.hpp"
#include "system/pipe.hpp"
#include "system/process.hpp"
#include "argument_visitor.hpp"
#include "expression_visitor.hpp"
#include "assignment_visitor.hpp"

int gorc::program_visitor::visit(pipe_command &cmd)
{
    // Open interprocess pipes
    size_t num_subcommands = cmd.subcommands->elements.size();

    std::vector<std::unique_ptr<pipe>> pipes;
    for(size_t i = 1; i < num_subcommands; ++i) {
        pipes.push_back(make_unique<pipe>());
    }

    std::vector<maybe<pipe*>> stdin_pipes;
    std::vector<maybe<pipe*>> stdout_pipes;

    // First process takes stdin from console
    stdin_pipes.push_back(nothing); // TODO: stdin redirection

    for(auto &pipe : pipes) {
        stdin_pipes.push_back(pipe.get());
        stdout_pipes.push_back(pipe.get());
    }

    // Last process sends stdout to console
    stdout_pipes.push_back(nothing); // TODO: stdout redirection

    auto sub_it = cmd.subcommands->elements.begin();
    auto stdin_it = stdin_pipes.begin();
    auto stdout_it = stdout_pipes.begin();

    std::vector<std::unique_ptr<process>> processes;

    while(sub_it != cmd.subcommands->elements.end() &&
          stdin_it != stdin_pipes.end() &&
          stdout_it != stdout_pipes.end()) {
        sexpr arg_list = ast_visit(argument_visitor(), (*sub_it)->arguments);
        std::vector<std::string> argv = argument_list_to_argv(arg_list);

        if(argv.empty()) {
            LOG_FATAL("cannot execute empty command");
        }

        // First token is the program to execute
        std::string prog = argv.front();
        std::vector<std::string> args;
        for(auto it = argv.begin() + 1;
            it != argv.end();
            ++it) {
            args.push_back(*it);
        }

        processes.push_back(make_unique<process>(prog,
                                                 args,
                                                 *stdin_it,
                                                 *stdout_it,
                                                 /* err */ nothing));
        ++sub_it;
        ++stdin_it;
        ++stdout_it;
    }

    int last_exit_code = 0;
    for(auto &proc : processes) {
        last_exit_code = proc->join();
    }

    return last_exit_code;
}

void gorc::program_visitor::visit(command_statement &cmd)
{
    int return_code = ast_visit(*this, *cmd.cmd);
    if(return_code != 0) {
        LOG_FATAL(format("command failed with code %d") % return_code);
    }
}

void gorc::program_visitor::visit(assignment_statement &s)
{
    sexpr value = ast_visit(argument_visitor(), s.value);

    ast_visit(assign_lvalue_visitor(value), *s.var);
}

void gorc::program_visitor::visit(var_declaration_statement &s)
{
    sexpr value;
    if(s.value.has_value()) {
        value = ast_visit(argument_visitor(), s.value.get_value());
    }

    create_variable(s.var->name, value);
}

void gorc::program_visitor::visit(if_statement &s)
{
    auto cond = ast_visit(expression_visitor(), *s.condition);
    if(as_boolean_value(cond)) {
        ast_visit(*this, *s.code);
    }
}

void gorc::program_visitor::visit(if_else_statement &s)
{
    auto cond = ast_visit(expression_visitor(), *s.condition);
    if(as_boolean_value(cond)) {
        ast_visit(*this, *s.code);
    }
    else {
        ast_visit(*this, *s.elsecode);
    }
}

void gorc::program_visitor::visit(return_statement &s)
{
    if(s.value.has_value()) {
        return_value = ast_visit(expression_visitor(), *s.value.get_value());
    }
    else {
        return_value = make_sexpr();
    }
}

void gorc::program_visitor::visit(call_statement &s)
{
    ast_visit(expression_visitor(), *s.value);
}

void gorc::program_visitor::visit(ast_list_node<statement*> &stmt_seq)
{
    for(auto &stmt : stmt_seq.elements) {
        // Abort executing a statement sequence after the first return statement
        if(return_value.has_value()) {
            return;
        }

        ast_visit(*this, *stmt);
    }
}

void gorc::program_visitor::visit(compound_statement &stmt)
{
    scoped_stack_frame sf;
    return ast_visit(*this, stmt.code);
}

void gorc::program_visitor::visit(translation_unit &tu)
{
    return ast_visit(*this, tu.code);
}

void gorc::program_visitor::visit(func_declaration_statement &)
{
    // Ignore function nodes during execution
}