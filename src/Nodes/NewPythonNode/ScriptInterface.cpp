//
// Created by bwll1 on 2024/9/30.
//
// bindings.cpp
#include <pybind11/pybind11.h>
//#include "PythonNodeModel.hpp"
//#include <QString>
int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(ScriptEngine, m) {
    m.doc() = "pybind11 example plugin";
    m.def("add", &add, "A function adds two numbers");
}