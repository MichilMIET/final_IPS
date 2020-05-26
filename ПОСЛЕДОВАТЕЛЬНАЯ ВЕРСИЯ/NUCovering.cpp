#include "fragmentation.h"
#include <chrono>
#include <ctime>
#include <iostream>
#include <locale.h>

/// параметры начальной прямоугольной области
const double g_l1_max = 12.0;
const double g_l2_max = g_l1_max;
const double g_l1_min = 8.0;
const double g_l2_min = g_l1_min;
const double g_l0 = 5.0;

/// точность аппроксимации рабочего пространства
const double g_precision = 0.25;


int main()
{
	setlocale(LC_ALL,"Rus");
	double x_min = -g_l1_max;
	double y_min = 0;
	double width = g_l1_max + g_l2_max + g_l0;
	double height = g_l1_max;

	high_level_analysis main_object(x_min, y_min, width, height);

	auto t1 = std::chrono::high_resolution_clock::now();
	main_object.GetSolution();
	auto t2 = std::chrono::high_resolution_clock::now();

	std::chrono::duration<double> resTime = (t2 - t1);
	std::cout << "Время выполнения " << resTime.count() << " секунд" << std::endl;;
	const char* out_files[3] = { "file1.txt", "file2.txt", "line.txt" };
	WriteResults( out_files );
	system("pause");
	return 0;
}