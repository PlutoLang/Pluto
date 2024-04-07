#pragma once

#define SCOPED_ENUM(name, type, ...) using name ## _t = type; struct name { enum _ : name ## _t { __VA_ARGS__ }; };
