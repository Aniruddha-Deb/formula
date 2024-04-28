#include <iostream>
#include <cctype>
#include <vector>
#include <queue>
#include <optional>
#include <utility>
#include <stack>
#include <memory>
#include <deque>
#include <fstream>
#include <sstream>
using namespace std;

int if_lbl_ctr = 0;

enum TokenType {
    NUMBER,
    IDENTIFIER,
    LBRACKET,
    RBRACKET,
    SEMICOLON,
    DEF,
    IF,
    THEN,
    ELSE,
    EQ,
    PLUS,
    MINUS,
    STAR,
    SLASH,
    COMMA
};

string toktype_to_str(TokenType type) {
    switch(type) {
        case NUMBER: return "NUMBER";
        case IDENTIFIER: return "IDENTIFIER";
        case LBRACKET: return "(";
        case RBRACKET: return ")";
        case SEMICOLON: return ";";
        case DEF: return "def";
        case EQ: return "=";
        case PLUS: return "+";
        case MINUS: return "-";
        case STAR: return "*";
        case SLASH: return "/";
        case COMMA: return ";";
        case IF: return "if";
        case THEN: return "then";
        case ELSE: return "else";
    }
}

union TokenData {
    const char* id;
    int num;
};

struct Token {
    TokenType type;
    variant<int, string> data;
    int line_no;
    int col_no;

    Token(TokenType _type) : type{_type}, data{0},
        line_no{0}, col_no{0} {}

    Token(TokenType _type, int _line_no, int _col_no) : type{_type}, data{0},
        line_no{_line_no}, col_no{_col_no} {}

    Token(TokenType _type, string _data, int _line_no, int _col_no) : 
        type{_type}, line_no{_line_no}, col_no{_col_no}, data{_data} {}

    Token(TokenType _type, int _data, int _line_no, int _col_no) : 
        type{_type}, line_no{_line_no}, col_no{_col_no}, data{_data} {}

    string to_string() {
        string str = toktype_to_str(type);
        if (type == NUMBER) str += " " + std::to_string(std::get<int>(data));
        if (type == IDENTIFIER) str += " " + std::get<string>(data);
        return str;
    }
};

enum Operator {
    ADD, SUB, MUL, DIV, EQUAL
};

string op2str(Operator op) {
    switch(op) {
        case ADD: return "ADD";
        case SUB: return "SUB";
        case MUL: return "MUL";
        case DIV: return "DIV";
        case EQUAL: return "EQUAL";
    }
}

struct ASTNode {
    virtual string to_string(string pad) { return ""; }
    virtual string code_gen(string pad) { return ""; }
};

struct Expression : ASTNode {
};

struct NullExpression : Expression {};

struct BinaryExpression : Expression {
    unique_ptr<Expression> lhs;
    Operator op;
    unique_ptr<Expression> rhs;

    BinaryExpression(unique_ptr<Expression>& _lhs, Operator _op, unique_ptr<Expression>& _rhs):
        lhs{std::move(_lhs)},
        op{_op},
        rhs{std::move(_rhs)} {}
    
    string to_string(string pad) {
        return op2str(op) + "\n" + 
               pad + "`- lhs: " + lhs->to_string(pad + "|  ") + "\n" +
               pad + "`- rhs: " + rhs->to_string(pad + "   ");
    }

    string code_gen(string pad) {
        string opc = "";
        switch(op) {
            case ADD: opc = "add"; break;
            case SUB: opc = "sub"; break;
            case MUL: opc = "mul"; break;
            case DIV: opc = "sdiv"; break;
            case EQUAL: opc = "cmp"; break;
        }
        stringstream s;
        s << lhs->code_gen(pad) << "\n" << pad;
        s << "str w8, [sp]\n" << pad; 
        s << "sub sp, sp, #16\n" << pad;
        s << rhs->code_gen(pad) << "\n" << pad;
        s << "ldr w9, [sp, #16]\n" << pad;
        if (op == EQUAL) s << "cmp w9, w8\n" << pad;
        else s << opc << " w8, w9, w8\n" << pad;
        s << "add sp, sp, #16";
        return s.str();
    }
};

struct IntLiteral : Expression {
    int value;

    IntLiteral(int _value) : value(_value) {}

    string to_string(string pad) {
        return std::to_string(value);
    }

    string code_gen(string pad) {
        return "mov w8, #" + std::to_string(value);
    }
};

struct Identifier : Expression {
    string value;
    int reg;

    Identifier(string _value, int _reg) : value(_value), reg(_reg) {}

    string to_string(string pad) {
        return value;
    }

    string code_gen(string pad) {
        return "mov w8, w" + std::to_string(reg);
    }
};

struct IfThenElseExpression : Expression {
    unique_ptr<Expression> cond_expr;
    unique_ptr<Expression> if_expr;
    unique_ptr<Expression> else_expr;

    IfThenElseExpression(unique_ptr<Expression>& _cond_expr, unique_ptr<Expression>& _if_expr):
        cond_expr{std::move(_cond_expr)},
        if_expr{std::move(_if_expr)},
        else_expr{nullptr} {}

    IfThenElseExpression(unique_ptr<Expression>& _cond_expr, unique_ptr<Expression>& _if_expr, unique_ptr<Expression>& _else_expr):
        cond_expr{std::move(_cond_expr)},
        if_expr{std::move(_if_expr)},
        else_expr{std::move(_else_expr)} {}

    string to_string(string pad) {
        if (else_expr.get() == nullptr) {
            return "if\n" + 
                   pad + "`- cond: " + cond_expr->to_string(pad + "|  ") + "\n" +
                   pad + "`- then: " + if_expr->to_string(pad + "   ");
        }
        else {
            return "if\n" + 
                   pad + "`- cond: " + cond_expr->to_string(pad + "|  ") + "\n" +
                   pad + "`- then: " + if_expr->to_string(pad + "|  ") + "\n" +
                   pad + "`- else: " + else_expr->to_string(pad + "   ");
        }
    }

    // TODO
    string code_gen(string pad) {
        if_lbl_ctr += 1;
        stringstream s;
        s << cond_expr->code_gen(pad) << "\n" << pad; // cond expr HAS to be eq
        s << "beq true_lbl_" << if_lbl_ctr << "\n" << pad;
        s << else_expr->code_gen(pad) << "\n" << pad;
        s << "b end_if_" << if_lbl_ctr << "\n";
        s << "true_lbl_" << if_lbl_ctr << ":\n" << pad;
        s << if_expr->code_gen(pad) << "\n";
        s << "end_if_" << if_lbl_ctr << ":\n";
        if_lbl_ctr -= 1;
        return s.str();
    }
};

struct FunctionApplicationExpression : Expression {
    string name;
    vector<unique_ptr<Expression>> parameters;

    FunctionApplicationExpression() {}

    string to_string(string pad) {
        string ret = name;
        for (int i=0; i<parameters.size()-1; i++) {
            ret += "\n" + pad + "`- " + parameters[i]->to_string(pad + "|  ");
        }
        if (!parameters.empty()) {
            ret += "\n" + pad + "`- " + parameters[parameters.size()-1]->to_string(pad + "   ");
        }
        return ret;
    }

    string code_gen(string pad) {
        stringstream s;
        s << "sub sp, sp, #32\n" << pad 
            // TODO push/pop args from stack
            // for now, save/restore the first 4 registers only
          << "stp x29, x30, [sp]\n" << pad
          << "stp w1, w2, [sp, #16]\n" << pad
          << "stp w3, w4, [sp, #24]\n" << pad;
        for (int i=0; i<parameters.size(); i++) {
            unique_ptr<Expression>& expr = parameters[i];
            s << expr->code_gen(pad) << "\n" << pad;
            s << "mov w" << i+1 << ", w8\n" << pad;
        }
        s << "bl _" << name << "\n" << pad;
        s << "ldp x29, x30, [sp]\n" << pad
          << "ldp w1, w2, [sp, #16]\n" << pad
          << "ldp w3, w4, [sp, #24]\n" << pad
          << "add sp, sp, #32";
        return s.str();
    }
};

struct FunctionDefinition : ASTNode {
    string name;
    vector<string> params;
    unique_ptr<Expression> value;

    FunctionDefinition(string _name, vector<string> _params, unique_ptr<Expression>& _value):
        name(_name),
        params(_params),
        value(std::move(_value)) {}

    FunctionDefinition() {}

    string to_string(string pad) {
        string ret = name + "(";
        for (string& param : params) {
            ret += param + ",";
        }
        if (ret[ret.size()-1] == ',') ret[ret.size()-1] = ')';
        else ret += ")";
        ret += "\n" + pad + "`- " + value->to_string(pad + "   ");
        return ret;
    }

    string code_gen(string pad) {
        stringstream s;
        s << "    .globl _" << name << "\n    .p2align 2\n"
          << "_" << name << ":\n"
          << "    " << value->code_gen("    ") << "\n";
          // << "    " << "mov w0, w8\n"
        if (name == "main") {
            // print the result of main (value in w8) by branching to _printf
            s << "    sub sp, sp, #32\n"
              << "    stp x29, x30, [sp, #16]\n"
              << "    add x29, sp, #16\n"
              << "    str x8, [sp]\n"
              << "    adrp x0, out_.str@PAGE\n"
              << "    add x0, x0, out_.str@PAGEOFF\n"
              << "    bl _printf\n"
              << "    ldp x29, x30, [sp, #16]\n"
              << "    add sp, sp, #32\n";
        }

        s << "    " << "ret\n";
        return s.str();
    }
};

std::queue<Token> tokens;
FunctionDefinition curr_fd;
std::stack<unique_ptr<Expression>> expr_stack;
std::vector<FunctionDefinition> definitions;

int tokenize(const string& input) {

    string id;
    bool tok_is_number = false;
    bool in_tok = false;

    bool in_expr = false;
    bool in_func_def = false;
    int line_no = 1;
    int col_no = 0;
    for (int i=0; i<input.length(); i++) {
        char c = input[i];
        if (c == '\t') col_no += 4;
        else col_no++;
        
        if (isnumber(c)) {
            if (in_tok && !tok_is_number) return -1;
            in_tok = true;
            tok_is_number = true;
            id += c;
        }
        else if (isalpha(c)) {
            if (in_tok && tok_is_number) return -1;
            in_tok = true;
            tok_is_number = false;
            id += c;
        }
        else {
            if (in_tok) {
                if (id == "def") {
                    tokens.emplace(DEF, line_no, col_no);
                }
                else if (id == "if") {
                    tokens.emplace(IF, line_no, col_no);
                }
                else if (id == "then") {
                    tokens.emplace(THEN, line_no, col_no);
                }
                else if (id == "else") {
                    tokens.emplace(ELSE, line_no, col_no);
                }
                else if (tok_is_number) {
                    tokens.emplace(NUMBER, stoi(id), line_no, col_no);
                }
                else {
                    tokens.emplace(IDENTIFIER, id.c_str(), line_no, col_no);
                }
            }
            in_tok = false;
            tok_is_number = false;
            id = "";
            if (isspace(c)) {
                if (c == '\n') {
                    line_no++;
                    col_no = 0;
                }
            }
            else {
                switch (c) {
                    case '(': tokens.emplace(LBRACKET, line_no, col_no); break;
                    case ')': tokens.emplace(RBRACKET, line_no, col_no); break;
                    case '=': tokens.emplace(EQ, line_no, col_no); break;
                    case '+': tokens.emplace(PLUS, line_no, col_no); break;
                    case '-': tokens.emplace(MINUS, line_no, col_no); break;
                    case ',': tokens.emplace(COMMA, line_no, col_no); break;
                    case '*': tokens.emplace(STAR, line_no, col_no); break;
                    case '/': tokens.emplace(SLASH, line_no, col_no); break;
                    case ';': tokens.emplace(SEMICOLON, line_no, col_no); break;
                    default : cout << "Lex Error: Bad Token at line " << line_no << ", col " << col_no << endl; return -1;
                }
            }
        }
    }

    return 0;
}

bool err_stack_empty() {
    cout << "Parse Error: Unexpected EOF" << endl;
    return false;
}

bool err_expr_queue_empty() {
    cout << "Parse Error: Expression queue empty" << endl;
    return false;
}

bool err_token_mismatch(TokenType toktype) {
    cout << "Parse Error: Expected token " << toktype_to_str(toktype) << 
        ", got " << tokens.front().to_string() << " at line " << 
        tokens.front().line_no << ", col " << tokens.front().col_no << endl;
    return false;
}

bool err_msg(string err) {
    if (!tokens.empty()) {
        cout << "Parse Error: " << err << " at line " << 
            tokens.front().line_no << ", col " << tokens.front().col_no << endl;
    }
    else {
        cout << "Parse Error: " << err << " at EOF" << endl;
    }
    return false;
}

bool empty() {
    return tokens.empty();
}

optional<Token> peek() {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    return tokens.front();
}

optional<Token> pop() {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    Token t = tokens.front();
    tokens.pop();
    return t;
}

optional<unique_ptr<Expression>> pop_expr() {
    if (expr_stack.empty()) {
        err_expr_queue_empty();
        return {};
    }
    unique_ptr<Expression> e = std::move(expr_stack.top());
    expr_stack.pop();
    return e;
}

optional<Token> popif(TokenType toktype) {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    if (tokens.front().type != toktype) {
        return {};
    }
    Token t = tokens.front();
    tokens.pop();
    return t;
}

optional<Token> peekif(TokenType toktype) {
    if (tokens.empty()) {
        err_stack_empty();
        return {};
    }
    if (tokens.front().type != toktype) {
        return {};
    }
    return tokens.front();
}

bool parse_expr();

bool parse_func_call_expr() {

    if (empty()) return err_stack_empty();

    if (peekif(LBRACKET)) {
        pop();
        if (!parse_expr()) return false;
        if (!popif(RBRACKET)) return err_token_mismatch(RBRACKET);
    }
    else if (peekif(IDENTIFIER)) {
        Token name = pop().value();

        if (peekif(LBRACKET)) {
            pop();
            unique_ptr<FunctionApplicationExpression> fae = make_unique<FunctionApplicationExpression>();
            fae->name = get<string>(name.data);
            while (!popif(RBRACKET)) {
                if (!parse_expr()) return false;
                fae->parameters.push_back(std::move(pop_expr().value()));

                if (!(popif(COMMA) || peekif(RBRACKET))) {
                    return err_msg("Expected token , or ), got " + peek()->to_string());
                }
            }
            expr_stack.push(std::move(fae));
        }
        else {
            // get identifier position in variables
            string symb = get<string>(name.data);
            int i;
            for (i=0; i<curr_fd.params.size(); i++) {
                if (curr_fd.params[i] == symb) break;
            }
            if (i == curr_fd.params.size()) {
                return err_msg("Could not find bound variable " + symb);
            }
            expr_stack.push(make_unique<Identifier>(get<string>(name.data), i+1));
        }
    }
    else if (peekif(NUMBER)) {
        int data = get<int>(pop().value().data);
        expr_stack.push(make_unique<IntLiteral>(data));
    }
    else {
        return err_msg("Expected number, identifier or (, got " + peek()->to_string());
    }
    return true;
}

bool parse_mul_div_expr() {

    if (!parse_func_call_expr()) return false;

    if (empty()) return err_stack_empty();

    if (peekif(RBRACKET) || peekif(SEMICOLON) || peekif(COMMA) || peekif(PLUS) || peekif(MINUS) || peekif(THEN) || peekif(ELSE) || peekif(EQ)) {
        // do nothing
    }
    else if (peekif(STAR) || peekif(SLASH)) {
        Operator op = pop().value().type == STAR ? MUL : DIV;
        if (!parse_mul_div_expr()) return false;
        unique_ptr<Expression> e1 = pop_expr().value();
        unique_ptr<Expression> e2 = pop_expr().value();
        expr_stack.push(make_unique<BinaryExpression>(e2, op, e1));
    }
    else {
        return err_msg("Expected ),;,,,+,-,*,/, got " + peek()->to_string());
    }

    return true;
}

bool parse_add_sub_expr() {

    if (!parse_mul_div_expr()) return false;

    if (empty()) return err_stack_empty();

    if (peekif(RBRACKET) || peekif(SEMICOLON) || peekif(COMMA) || peekif(THEN) || peekif(ELSE) || peekif(EQ)) {
        // do nothing
    }
    else if (peekif(PLUS) || peekif(MINUS)) {
        Operator op = pop().value().type == PLUS ? ADD : SUB;
        if (!parse_expr()) return false;
        unique_ptr<Expression> e1 = pop_expr().value();
        unique_ptr<Expression> e2 = pop_expr().value();
        expr_stack.push(make_unique<BinaryExpression>(e2, op, e1));
    }
    else {
        return err_msg("Expected ),;,,,+,-, got " + peek()->to_string());
    }

    return true;
}

bool parse_eq_expr() {

    if (!parse_add_sub_expr()) return false;

    if (empty()) return err_stack_empty();

    if (peekif(RBRACKET) || peekif(SEMICOLON) || peekif(COMMA) || peekif(ELSE) || peekif(THEN)) {
        // do nothing
    }
    else if (peekif(EQ)) {
        pop();
        Operator op = EQUAL;
        if (!parse_expr()) return false;
        unique_ptr<Expression> e1 = pop_expr().value();
        unique_ptr<Expression> e2 = pop_expr().value();
        expr_stack.push(make_unique<BinaryExpression>(e2, op, e1));
    }
    else {
        return err_msg("Expected ),;,,,= got " + peek()->to_string());
    }

    return true;
}

bool parse_expr() {

    if (!peekif(IF)) return parse_eq_expr();

    pop(); // pop if
    if (!parse_eq_expr()) return err_msg("If statement condition missing");
    unique_ptr<Expression> cond = pop_expr().value();
    if (!popif(THEN)) return err_msg("Expected then");
    if (!parse_expr()) return err_msg("If statement true branch missing");
    unique_ptr<Expression> if_expr = pop_expr().value();
    if (!peekif(ELSE)) {
        expr_stack.push(make_unique<IfThenElseExpression>(cond, if_expr));
        return true;
    }
    pop(); // pop else
    if (!parse_expr()) return err_msg("If statement false branch missing");
    unique_ptr<Expression> else_expr = pop_expr().value();
    expr_stack.push(make_unique<IfThenElseExpression>(cond, if_expr, else_expr));
    return true;

}

bool parse_func_defn() {
    if (!peekif(IDENTIFIER)) return err_token_mismatch(IDENTIFIER);
    curr_fd.name = get<string>(pop().value().data);
    if (!popif(LBRACKET)) return err_token_mismatch(LBRACKET);
    while (!popif(RBRACKET)) {
        if (!peekif(IDENTIFIER)) {
            return err_token_mismatch(IDENTIFIER);
        }
        curr_fd.params.push_back(get<string>(pop().value().data));
        if (!(popif(COMMA) || peekif(RBRACKET))) {
            return err_msg("Expected token , or ), got " + peek()->to_string());
        }
    }
    return true;
}

bool parse() {

    while (!empty()) {
        if (!popif(DEF)) return err_token_mismatch(DEF);
        if (!parse_func_defn()) return false;
        if (!popif(EQ)) return err_token_mismatch(EQ);
        if (!parse_expr()) return false;
        if (!popif(SEMICOLON)) return err_token_mismatch(SEMICOLON);

        unique_ptr<Expression> expr = pop_expr().value();
        definitions.emplace_back(curr_fd.name, curr_fd.params, expr);
        curr_fd.params.clear();
    }

    return true;
}

int main(int argc, char** argv) {

    if (argc != 2) {
        cout << "Usage: formula <filename>" << endl;
        return 0;
    }

    std::ifstream t(argv[1]);
    std::stringstream buffer;
    buffer << t.rdbuf();

    tokenize(buffer.str());
    parse();

    cout << "    .section    __TEXT,__text,regular,pure_instructions" << endl;

    for (auto& def : definitions) {
        cout << def.code_gen("") << endl;
    }

    // main would be the last function to be generated
    cout << endl;
    cout << "    .section    __TEXT,__cstring,cstring_literals" << endl;
    cout << "out_.str:" << endl;
    cout << "    .asciz \"%d\\n\"" << endl;

    return 0;

}
