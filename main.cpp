// #include <iostream>
// #include <chrono>
// #include "piecewise.hpp"
// #include "piecewise_map.hpp"

// using namespace std;
// using namespace std::chrono;

// // Fonction de benchmark générique
// template<typename Func>
// long long benchmark(Func f, int repeat = 5) {
//     long long total = 0;
//     for (int i = 0; i < repeat; i++) {
//         auto start = high_resolution_clock::now();
//         f();  // La lambda crée et manipule les objets
//         auto end = high_resolution_clock::now();
//         total += duration_cast<microseconds>(end - start).count();
//     }
//     return total / repeat;
// }

// int main() {
//     // Benchmark version "map" avec sum
//     auto t_map = benchmark([]() {
//         auto g1 = map_version::delta_profile(5, 10, 20, 30);
//         auto g2 = map_version::cba_profile(10, 15, 25);
//         g1.sum(g2);
//         return g1;
//     });

//     // Benchmark version "list" avec add
//     auto t_list = benchmark([]() {
//         auto f1 = list_version::cba_profile(10, 15, 25, 1000);
//         auto f2 = list_version::delta_profile_temp(5, 10, 20, 30, 1000);
//         f1.add(f2);
//         return f1;
//     });

//     cout << "Temps moyen sum (map_version)  : " << t_map << " µs\n";
//     cout << "Temps moyen add (list_version) : " << t_list << " µs\n";

//     return 0;
// }

#include <iostream>
#include <chrono>
#include <vector>
#include <fstream>
#include "piecewise.hpp"
#include "piecewise_map.hpp"

using namespace std;
using namespace std::chrono;

// ==================== Génération zigzag ====================
map_version::PiecewiseLinearFunction zigzag_map(int x_max, double y_min, double y_max, int period) {
    map_version::PiecewiseLinearFunction f(y_min);
    for (int x = period; x <= x_max; x += period) {
        double y = ((x / period) % 2 == 0) ? y_min : y_max;

        double delta  = ((x / period) % 2 == 0) ? y_min - y_max : y_max -y_min;
        
        f.addBreakpoint(x, delta);
        // cout << "breakpoint: last_x=" <<  x << " last_y=" << y   << endl;
    }
    return f;
}

list_version::PiecewiseLinearFunction zigzag_list(int x_max, double y_min, double y_max, int period) {
    list_version::PiecewiseLinearFunction f;
    for (int x = period; x <= x_max; x += period) {
        double y = ((x / period) % 2 == 0) ? y_min : y_max;
        double y_first = ((x / period) % 2 == 0) ? y_max : y_min;

        auto seg = std::make_shared<list_version::Segment>(x-period, y_first , x, y);
        f.add_segment(seg);
       //  cout << "segment: last_x=" <<  x-period << " last_y=" << y_first << " x =" << x << " y=" << y << endl;
    
   
    }

    return f;
}

// list_version::PiecewiseLinearFunction zigzag_list(int x_max, double y_min, double y_max, int period) {
//     list_version::PiecewiseLinearFunction f;
//     double last_x = 0;
//     double last_y = y_min; // début à y_min
//     bool toggle = true;     // alterne entre y_min et y_max

//     for (int x = period; x <= x_max; x += period) {
//         double y = toggle ? y_max : y_min;
//         auto seg = std::make_shared<list_version::Segment>(last_x, last_y, x, y);
//         f.add_segment(seg);

//         last_x = x;
//         last_y = y;
//         toggle = !toggle; // inverse pour le prochain segment

//         cout << "segment: last_x=" << last_x << " last_y=" << last_y
//              << " x=" << x << " y=" << y << endl;
//     }
//     return f;
// }


// ==================== Génération delta ====================
map_version::PiecewiseLinearFunction delta_map(int x_max, int width, double amplitude) {
    map_version::PiecewiseLinearFunction g;
    int mid = x_max / 2;
    int left = mid - width / 2;
    int right = mid + width / 2;
    g.addBreakpoint(left, 0);
    g.addBreakpoint(mid, amplitude);
    g.addBreakpoint(right, -amplitude);
    return g;
}

list_version::PiecewiseLinearFunction delta_list(int x_max, int width, double amplitude) {
    list_version::PiecewiseLinearFunction g;
    int mid = x_max / 2;
    int left = mid - width / 2;
    int right = mid + width / 2;
    auto seg1 = std::make_shared<list_version::Segment>(0, 0, left, 0);
    auto seg2 = std::make_shared<list_version::Segment>(left, 0, mid, amplitude);
    auto seg3 = std::make_shared<list_version::Segment>(mid, amplitude, right, 0);
    auto seg4 = std::make_shared<list_version::Segment>(right, 0, x_max, 0);
    g.add_segment(seg1);
    g.add_segment(seg2);
    g.add_segment(seg3);
    g.add_segment(seg4);
    return g;
}

// ==================== Benchmark ====================
template<typename Func>
long long benchmark(Func f, int repeat = 1) {
    long long total = 0;
    for (int i = 0; i < repeat; i++) {
        auto start = high_resolution_clock::now();
        f();
        auto end = high_resolution_clock::now();
        total += duration_cast<milliseconds>(end - start).count();
    }
    return total / repeat;
}

int main() {
    int x_max =4000;
    double y_min = 10;
    double y_max = 20;
    int period = 1;
    int delta_max_width = x_max;
    double amplitude = 50;
     int mid = x_max / 2;


    auto f_map = zigzag_map(x_max, y_min, y_max, period);
    auto f_list = zigzag_list(x_max, y_min, y_max, period);

    ofstream out("timing_comparison.csv");
    out << "width,time_map_us,time_list_us,nodes_in_g\n";

    for (int width = 10; width <= delta_max_width; width += 10) {
        auto g_map = delta_map(x_max, width, amplitude);
        auto g_list = delta_list(x_max, width, amplitude);

        // nodes de f_map dans la largeur de g_map

        int left = mid - width / 2;
        int right = mid + width / 2;
        auto points = f_map.to_points_cumulative();
        int nodes_in_g = 0;
        for (auto& p : points) if (p.first >= left && p.first <= right) nodes_in_g++;

        // Benchmark map
        long long t_map = benchmark([&]() {
            auto tmp = f_map;
            tmp.sum(g_map);
        });

        // Benchmark list
        long long t_list = benchmark([&]() {
            auto tmp = f_list;
            tmp.add(g_list);
        });

        out << width << "," << t_map << "," << t_list << "," << nodes_in_g << "\n";
        cout << "Width=" << width << " map=" << t_map << " list=" << t_list
             << " nodes_in_g=" << nodes_in_g << endl;
    }

    out.close();
    cout << "Données exportées vers timing_comparison.csv" << endl;
    return 0;
}
