#ifndef WINAPI_INTEROP_HPP_INCLUDED
#define WINAPI_INTEROP_HPP_INCLUDED

#include <vector>
#include "process_info.hpp"

process_info process_id_to_process_info(DWORD processID);
std::vector<process_info> get_process_infos();

#endif // WINAPI_INTEROP_HPP_INCLUDED
