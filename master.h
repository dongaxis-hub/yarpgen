/*
Copyright (c) 2015-2016, Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

//////////////////////////////////////////////////////////////////////////////

#pragma once

#include <random>
#include <algorithm>
#include <fstream>

#include "type.h"
#include "node.h"

struct GenerationPolicy {
    std::string ext_num;

    std::vector<Type::TypeID> allowed_types;

    int min_arr_num;
    int max_arr_num;
    int min_arr_size;
    int max_arr_size;
    Array::Ess primary_ess;

    uint64_t min_var_val;
    uint64_t max_var_val;

    int max_arith_depth;

    bool self_dep;
    bool inter_war_dep;

    bool else_branch;
};

class RandValGen {
    public:
        RandValGen (uint64_t _seed);
        template<typename T>
        T get_rand_value (T from, T to);

    private:
        uint64_t seed;
        std::mt19937_64 rand_gen;
};

extern std::shared_ptr<RandValGen> rand_val_gen;

class Master {
    public:
        Master (std::string _out_folder);
        void generate ();
        std::string emit_func ();
        std::string emit_init ();
        std::string emit_decl ();
        std::string emit_hash ();
        std::string emit_check ();
        std::string emit_main ();

    private:
        void write_file (std::string of_name, std::string data);
        std::string emit_loop (std::shared_ptr<Data> arr, std::shared_ptr<FuncCallExpr> func_call = NULL);

        GenerationPolicy gen_policy;
        std::vector<std::shared_ptr<Stmt>> program;
        std::vector<std::shared_ptr<Data>> inp_sym_table;
        std::vector<std::shared_ptr<Data>> out_sym_table;
        std::string out_folder;
};

class Gen {
    public:
        Gen (GenerationPolicy _gen_policy) : gen_policy (_gen_policy) {}
        void set_gen_policy (GenerationPolicy _gen_policy) { gen_policy = _gen_policy; }
        std::vector<std::shared_ptr<Stmt>>& get_program () { return program; }
        std::vector<std::shared_ptr<Data>>& get_sym_table () { return sym_table; }
        virtual void generate () = 0;

    protected:
        GenerationPolicy gen_policy;
        std::vector<std::shared_ptr<Stmt>> program;
        std::vector<std::shared_ptr<Data>> sym_table;
};

class ArrayGen : public Gen {
    public:
        ArrayGen (GenerationPolicy _gen_policy) : Gen (_gen_policy) {}
        std::shared_ptr<Array> get_inp_arr () { return inp_arr; }
        std::shared_ptr<Array> get_out_arr () { return out_arr; }
        void generate ();

    private:
        Array::Ess primary_ess;
        std::shared_ptr<Array> inp_arr;
        std::shared_ptr<Array> out_arr;
};

class LoopGen : public Gen {
    public:
        LoopGen (GenerationPolicy _gen_policy) : Gen (_gen_policy), min_ex_arr_size (UINT64_MAX) {}
        std::vector<std::shared_ptr<Data>>& get_inp_sym_table () { return inp_sym_table; }
        std::vector<std::shared_ptr<Data>>& get_out_sym_table () { return out_sym_table; }
        void generate ();

    private:
        std::vector<std::shared_ptr<Stmt>> body_gen (std::vector<std::shared_ptr<Expr>> inp_expr, std::vector<std::shared_ptr<Expr>> out_expr);
        uint64_t min_ex_arr_size;
        std::vector<std::shared_ptr<Data>> inp_sym_table;
        std::vector<std::shared_ptr<Data>> out_sym_table;
};

class VarValGen : public Gen {
    public:
        VarValGen (GenerationPolicy _gen_policy, Type::TypeID _type_id) : Gen (_gen_policy), type_id(_type_id), val(0) {}
        void generate();
        void generate_step();
        uint64_t get_value () { return val; }

    private:
        Type::TypeID type_id;
        uint64_t val;
};

class StepExprGen : public Gen {
    public:
        StepExprGen (GenerationPolicy _gen_policy, std::shared_ptr<Variable> _var, int64_t _step) :
                     Gen (_gen_policy), var(_var), step(_step), expr(NULL) {}
        void generate ();
        std::shared_ptr<Expr> get_expr () { return expr; }

    private:
        int64_t step;
        std::shared_ptr<Expr> expr;
        std::shared_ptr<Variable> var;
};

class TripGen : public Gen {
    public:
        TripGen (GenerationPolicy _gen_policy, uint64_t _min_ex_arr_size) : Gen (_gen_policy), min_ex_arr_size(_min_ex_arr_size) {}
        void generate ();
        std::shared_ptr<Variable> get_iter () { return iter; }
        std::shared_ptr<DeclStmt> get_iter_decl ();
        std::shared_ptr<Expr> get_step_expr () { return step_expr; }
        std::shared_ptr<Expr> get_cond () { return cond; }

    private:
        void gen_condition (bool hit_end, int64_t step);
        std::shared_ptr<Variable> iter;
        uint64_t min_ex_arr_size;
        std::shared_ptr<Expr> step_expr;
        std::shared_ptr<Expr> cond;
};

class ArithExprGen : public Gen {
    public:
        ArithExprGen (GenerationPolicy _gen_policy,
                      std::vector<std::shared_ptr<Expr>> _inp,
                      std::shared_ptr<Expr> _out) :
                      Gen (_gen_policy), inp(_inp), out(_out) {}
        void generate();
        std::shared_ptr<Stmt> get_expr_stmt ();
        std::shared_ptr<Expr> get_expr () { return res_expr; };

    private:
        std::shared_ptr<Expr> rebuild_unary(Expr::UB ub, std::shared_ptr<Expr> expr);
        std::shared_ptr<Expr> rebuild_binary(Expr::UB ub, std::shared_ptr<Expr> expr);
        std::shared_ptr<Expr> gen_level (int depth);
        std::vector<std::shared_ptr<Expr>> inp;
        std::shared_ptr<Expr> out;
        std::shared_ptr<Expr> res_expr;
};
