#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " plan.txt K\n";
        return 1;
    }

    std::string ruta = argv[1];
    std::ifstream archivo(ruta);
    if (!archivo) {
        std::cerr << "Error: no se pudo abrir " << ruta << "\n";
        return 1;
    }

    std::string linea;
    while (std::getline(archivo, linea)) {
        std::cout << linea << "\n";
    }

    return 0; 
}