#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <unordered_map>
#include <random>
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>

struct Actividad {
    std::string id;
    std::string nombre;
    int tiempo_ms;
    std::vector<std::string> dependencias;

    int deps_pendientes = 0;
    std::vector<int> dependientes;
    bool lanzada = false;
    bool abortada = false;

    int buzon_r = -1;
    int buzon_w = -1;
};

std::string limpiar(const std::string& s) {
    size_t a = s.find_first_not_of(" \t");
    size_t b = s.find_last_not_of(" \t");
    if (a == std::string::npos) return "";
    return s.substr(a, b - a + 1);
}

volatile sig_atomic_t seremi_llego = 0;

void manejador_sigint(int) {
    seremi_llego = 1;
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Uso: " << argv[0] << " plan.txt K\n";
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = manejador_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, nullptr);

    
    const char* env_falla = std::getenv("FALLA_ID");
    std::string falla_id = env_falla ? std::string(env_falla) : "";

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

    std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(100, 5000);
    for (auto& a : actividades) {
        if (a.tiempo_ms == -1) a.tiempo_ms = dist(gen);
    }

    std::unordered_map<std::string, int> indice;
    for (size_t i = 0; i < actividades.size(); i++) {
        indice[actividades[i].id] = i;
    }

    for (size_t i = 0; i < actividades.size(); i++) {
        actividades[i].deps_pendientes = actividades[i].dependencias.size();
        for (const auto& dep : actividades[i].dependencias) {
            int pos = indice[dep];
            actividades[pos].dependientes.push_back(i);
        }
    }

    int K = std::stoi(argv[2]);
    int vivos = 0;
    int terminadas = 0;

    std::vector<int> listas;
    for (size_t i = 0; i < actividades.size(); i++) {
        if (actividades[i].deps_pendientes == 0) listas.push_back(i);
    }

    std::unordered_map<pid_t, int> pid_a_pos;

    while (terminadas < (int)actividades.size()) {

        if (seremi_llego) {
            std::cout << "\n[SEREMI] Llego la autoridad! Abortando todas las actividades...\n";
            for (auto& par : pid_a_pos) {
                kill(par.first, SIGKILL);
            }
            while (wait(nullptr) > 0) {}
            std::cout << "[SEREMI] Celebracion clausurada.\n";
            return 1;
        }

        while (!listas.empty() && vivos < K) {
            int pos = listas.back();
            listas.pop_back();
            if (actividades[pos].lanzada || actividades[pos].abortada) continue;
            actividades[pos].lanzada = true;

            for (int dep : actividades[pos].dependientes) {
                if (actividades[dep].buzon_r == -1) {
                    int fd[2];
                    if (pipe(fd) == -1) {
                        std::cerr << "Error al crear pipe\n";
                        _exit(1);
                    }
                    actividades[dep].buzon_r = fd[0];
                    actividades[dep].buzon_w = fd[1];
                }
            }

            std::cout.flush();
            pid_t pid = fork();
            if (pid == 0) {
              std::cout << "[INICIO] id=" << actividades[pos].id << " "
          << actividades[pos].nombre
          << " (" << actividades[pos].tiempo_ms << "ms)\n";
                usleep(actividades[pos].tiempo_ms * 1000);

                
                if (!falla_id.empty() && actividades[pos].id == falla_id) {
                    std::cout << "[FALLO]  " << actividades[pos].nombre << " fallo internamente\n";
                    std::cout.flush();
                    _exit(1);
                }

                std::cout << "[FIN]    id=" << actividades[pos].id << " "
          << actividades[pos].nombre << "\n";

                std::string msg = actividades[pos].nombre + " listo\n";
                for (int dep : actividades[pos].dependientes) {
                    write(actividades[dep].buzon_w, msg.c_str(), msg.size());
                }
                std::cout.flush();
                _exit(0);
            } else {
                vivos++;
                pid_a_pos[pid] = pos;
            }
        }

        if (vivos > 0) {
            int estado;
            pid_t fin = waitpid(-1, &estado, 0);
            if (fin == -1) {
                continue;
            }
            vivos--;
            terminadas++;

            int pos = pid_a_pos[fin];
            pid_a_pos.erase(fin);

            bool fallo = !WIFEXITED(estado) || WEXITSTATUS(estado) != 0;

            if (fallo) {
               
                std::cout << "[ABORTO] rama de " << actividades[pos].nombre << " cancelada\n";
                std::vector<int> cola = actividades[pos].dependientes;
                while (!cola.empty()) {
                    int x = cola.back();
                    cola.pop_back();
                    if (actividades[x].abortada || actividades[x].lanzada) continue;
                    actividades[x].abortada = true;
                    terminadas++;
                    std::cout << "   -> abortada: " << actividades[x].nombre << "\n";
                    if (actividades[x].buzon_r != -1) {
                        close(actividades[x].buzon_r);
                        close(actividades[x].buzon_w);
                        actividades[x].buzon_r = -1;
                        actividades[x].buzon_w = -1;
                    }
                    for (int y : actividades[x].dependientes) cola.push_back(y);
                }
            } else {
                
                for (int dep : actividades[pos].dependientes) {
                    actividades[dep].deps_pendientes--;
                    if (actividades[dep].deps_pendientes == 0 && !actividades[dep].abortada) {
                        if (actividades[dep].buzon_r != -1) {
                            std::string acumulado;
                            char buf[256];
                            int esperados = actividades[dep].dependencias.size();
                            int recibidos = 0;
                            while (recibidos < esperados) {
                                int n = read(actividades[dep].buzon_r, buf, sizeof(buf) - 1);
                                if (n <= 0) break;
                                buf[n] = '\0';
                                acumulado += buf;
                                recibidos = std::count(acumulado.begin(), acumulado.end(), '\n');
                            }
                            std::cout << "[PIPE] " << actividades[dep].nombre
                                      << " recibio: " << acumulado;
                            close(actividades[dep].buzon_r);
                            close(actividades[dep].buzon_w);
                            actividades[dep].buzon_r = -1;
                            actividades[dep].buzon_w = -1;
                        }
                        listas.push_back(dep);
                    }
                }
            }
        }
    }

    std::cout << "Todas las actividades completadas.\n";
    return 0;
}