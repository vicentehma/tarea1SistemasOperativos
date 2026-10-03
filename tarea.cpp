#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

struct Actividad {
    std::string id;
    std::string nombre;
    int tiempo_ms;
    std::vector<std::string> dependencias;
};


std::string limpiar(const std::string& s) {
    size_t inicio = s.find_first_not_of(" \t");
    size_t fin = s.find_last_not_of(" \t");
    if (inicio == std::string::npos) return "";
    return s.substr(inicio, fin - inicio + 1);
}

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

    std::vector<Actividad> actividades;
    std::string linea;

    while (std::getline(archivo, linea)) {
        if (limpiar(linea).empty()) continue;

        std::stringstream ss(linea);
        std::string campo;
        Actividad act;

        std::getline(ss, campo, ':');
        act.id = limpiar(campo);

        std::getline(ss, campo, ':');
        act.nombre = limpiar(campo);

        std::getline(ss, campo, ':');
        std::string tiempoStr = limpiar(campo);
        if (tiempoStr.empty()) {
            act.tiempo_ms = -1;
        } else {
            act.tiempo_ms = std::stoi(tiempoStr);
        }

        std::getline(ss, campo, ':');
        std::stringstream depss(campo);
        std::string dep;
        while (std::getline(depss, dep, ',')) {
            std::string d = limpiar(dep);
            if (!d.empty()) act.dependencias.push_back(d);
        }

        actividades.push_back(act);
    }

    std::cout << "Se leyeron " << actividades.size() << " actividades:\n";
    for (const auto& a : actividades) {
        std::cout << "ID=" << a.id
                  << " | Nombre=" << a.nombre
                  << " | Tiempo=" << a.tiempo_ms
                  << " | Deps=";
        for (const auto& d : a.dependencias) std::cout << d << " ";
        std::cout << "\n";
    }

    return 0;
}