#ifndef FILEREADER_H_INCLUDED
#define FILEREADER_H_INCLUDED

#include <string>
#include <memory>

#include "mappers/mapper.h"

std::shared_ptr<Mapper> read_file(std::string filename);

#endif // FILEREADER_H_INCLUDED
