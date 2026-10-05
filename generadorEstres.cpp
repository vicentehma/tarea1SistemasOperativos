#include <iostream>
#include <fstream>
#include <string>
 
int main(int argc, char* argv[]) {
    int N          = (argc > 1) ? std::stoi(argv[1]) : 10000;
    int grupos     = (argc > 2) ? std::stoi(argv[2]) : 100;
    std::string salida = (argc > 3) ? argv[3] : "estres.txt";
 
    if (grupos < 1) grupos = 1;
    int tam_grupo = N / grupos;
    if (tam_grupo < 1) tam_grupo = 1;
 
    std::ofstream f(salida);
    if (!f) {
        std::cerr << "Error: no se pudo crear " << salida << "\n";
        return 1;
    }
 
    for (int i = 1; i <= N; i++) {
        std::string dep;
    
        if ((i - 1) % tam_grupo != 0) {
            dep = std::to_string(i - 1);
        }
        
        f << i << " : actividad_" << i << " : 1 : " << dep << "\n";
    }
 
    std::cout << "Generado '" << salida << "' con " << N
              << " actividades en ~" << grupos << " grupos.\n";
    return 0;
}