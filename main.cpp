#include <iostream>
#include <chrono>
#include "piecewise.hpp"
#include "piecewise_map.hpp"

using namespace std;
using namespace std::chrono;

// Fonction de benchmark générique
template<typename Func>
long long benchmark(Func f, int repeat = 5) {
    long long total = 0;
    for (int i = 0; i < repeat; i++) {
        auto start = high_resolution_clock::now();
        f();  // La lambda crée et manipule les objets
        auto end = high_resolution_clock::now();
        total += duration_cast<microseconds>(end - start).count();
    }
    return total / repeat;
}

int main() {
    // Benchmark version "map" avec sum
    auto t_map = benchmark([]() {
        auto g1 = map_version::delta_profile(5, 10, 20, 30);
        auto g2 = map_version::cba_profile(10, 15, 25);
        g1.sum(g2);
        return g1;
    });

    // Benchmark version "list" avec add
    auto t_list = benchmark([]() {
        auto f1 = list_version::cba_profile(10, 15, 25, 1000);
        auto f2 = list_version::delta_profile_temp(5, 10, 20, 30, 1000);
        f1.add(f2);
        return f1;
    });

    cout << "Temps moyen sum (map_version)  : " << t_map << " µs\n";
    cout << "Temps moyen add (list_version) : " << t_list << " µs\n";

    return 0;
}
