#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <random>

struct Actividad {
    std::string id;
    std::string nombre;
    int tiempo_ms;
    std::vector<std::string> dependencias;
};

std::string limpiar(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    size_t b = s.find_last_not_of(" \t");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " plan.txt K\n";
        return 1;
    }

    std::ifstream archivo(argv[1]);
    if (!archivo) {
        std::cerr << "Error: no se pudo abrir " << argv[1] << "\n";
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
        std::string t = limpiar(campo);
        act.tiempo_ms = t.empty() ? -1 : std::stoi(t);

        std::getline(ss, campo, ':');
        std::stringstream deps(campo);
        std::string d;
        while (std::getline(deps, d, ',')) {
            d = limpiar(d);
            if (!d.empty()) act.dependencias.push_back(d);
        }

        actividades.push_back(act);
    }
    // el enunciado pide entre 100 y 5000 ms para las que no traen tiempo
    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(100, 5000);
    for (auto& a : actividades) {
        if (a.tiempo_ms == -1) a.tiempo_ms = dist(gen);
    }

    std::unordered_map<std::string, int> indice;
    for (size_t i = 0; i < actividades.size(); i++) {
        indice[actividades[i].id] = i;
    }

    std::cout << "Se leyeron " << actividades.size() << " actividades:\n";
    for (const auto& a : actividades) {
        std::cout << a.id << " " << a.nombre << " " << a.tiempo_ms << " | ";
        for (const auto& dep : a.dependencias) std::cout << dep << " ";
        std::cout << "\n";
    }

    return 0;
}