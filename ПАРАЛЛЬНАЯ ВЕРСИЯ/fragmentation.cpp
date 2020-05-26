#include "fragmentation.h"

cilk::reducer< cilk::op_vector<Box> > solution;
cilk::reducer< cilk::op_vector<Box> > not_solution;
cilk::reducer< cilk::op_vector<Box> > boundary;
cilk::reducer< cilk::op_vector<Box> > temporary_boxes;



/// ������� gj()
//------------------------------------------------------------------------------------------
double g1(double x1, double x2)
{
	return (x1 * x1 + x2 * x2 - g_l1_max * g_l1_max);
}

//------------------------------------------------------------------------------------------
double g2(double x1, double x2)
{
	return (g_l1_min * g_l1_min - x1 * x1 - x2 * x2);
}

//------------------------------------------------------------------------------------------
double g3(double x1, double x2)
{
	return (x1 * x1 + x2 * x2 - g_l2_max * g_l2_max);
}

//------------------------------------------------------------------------------------------
double g4(double x1, double x2)
{
	return (g_l2_min * g_l2_min - x1 * x1 - x2 * x2);
}


//------------------------------------------------------------------------------------------
low_level_fragmentation::low_level_fragmentation(double& min_x, double& min_y, double& x_width, double& y_height)
{
	current_box = Box(min_x, min_y, x_width, y_height);
}

//------------------------------------------------------------------------------------------
low_level_fragmentation::low_level_fragmentation(const Box& box)
{
	current_box = box;
}

// ������� VerticalSplitter() ��������� ������� �������� box �� ������
void low_level_fragmentation::VerticalSplitter(const Box& box, boxes_pair& vertical_splitter_pair)
{
	double min_x, min_y, x_width, y_height;
	box.GetParameters(min_x, min_y, x_width, y_height);
	Box tmp_box_1 = Box(min_x, min_y, x_width / 2, y_height);
	Box tmp_box_2 = Box(min_x + x_width / 2, min_y, x_width / 2, y_height);
	vertical_splitter_pair = std::pair<Box, Box>(tmp_box_1, tmp_box_2);
}

// ������� HorizontalSplitter() ��������� ������� �������� box �� ������
void low_level_fragmentation::HorizontalSplitter(const Box& box, boxes_pair& horizontal_splitter_pair)
{
	double min_x, min_y, x_width, y_height;
	box.GetParameters(min_x, min_y, x_width, y_height);
	Box tmp_box_1 = Box(min_x, min_y, x_width, y_height / 2);
	Box tmp_box_2 = Box(min_x, min_y + y_height / 2, x_width, y_height / 2);
	horizontal_splitter_pair = std::pair<Box, Box>(tmp_box_1, tmp_box_2);
}

//������� GetNewBoxes() ��������� ������� �������� box �� ������� �������, ������� HorizontalSplitter() ��� VerticalSplitter()
void low_level_fragmentation::GetNewBoxes(const Box& box, boxes_pair& new_pair_of_boxes)
{
	double x_width;
	double y_height;
	box.GetWidhtHeight(x_width, y_height);
	if (x_width > y_height) {
		VerticalSplitter(box, new_pair_of_boxes);
	}
	else {
		HorizontalSplitter(box, new_pair_of_boxes);
	}
}

//------------------------------------------------------------------------------------------
unsigned int low_level_fragmentation::FindTreeDepth()
{
	double box_diagonal = current_box.GetDiagonal();

	if (box_diagonal <= g_precision)
	{
		return 0;
	}
	else
	{
		boxes_pair new_boxes;
		// ��������, �������� ��������� ������� �� ������
		VerticalSplitter(current_box, new_boxes);
		unsigned int tree_depth = 1;

		box_diagonal = new_boxes.first.GetDiagonal();

		if (box_diagonal <= g_precision)
		{
			return tree_depth;
		}
		else
		{
			for (;;)
			{
				GetNewBoxes(new_boxes.first, new_boxes);
				++tree_depth;
				box_diagonal = new_boxes.first.GetDiagonal();

				if (box_diagonal <= g_precision)
				{
					break;
				}
			}
			return tree_depth;
		}
	}
}

// ������� ClasifyBox() ����������� box � �������������� ���
int low_level_fragmentation::ClasifyBox(const min_max_vectors& vects)
{
	int j = 0;
	while (j < vects.first.size())
	{
		double current_value = vects.first[j];
		if (current_value > 0)
			return 1;
		j++;
	}

	j = 0;
	while (j < vects.second.size())
	{
		double current_value = vects.second[j];
		if (current_value > 0)
			return 2;
		j++;
	}
	return 3;
}

// ������� GetBoxType() ��������� ������������������ ����� box �� ��������� �������, ��� ������� ��� �� �������, ��� ��������� ��� � ��������� �������, ��� ������� ��� � ���, ��� �������� ����������� �������
void low_level_fragmentation::GetBoxType(const Box& box)
{
	min_max_vectors vects;
	GetMinMax(box, vects);
	int box_type = ClasifyBox(vects);
	switch (box_type)
	{
	case 1:
		not_solution->push_back(box);
		break;

	case 2:
	{
		boxes_pair new_boxes;
		GetNewBoxes(box, new_boxes);
		if (new_boxes.first.GetDiagonal() < g_precision) {
			boundary->push_back(new_boxes.first);
			boundary->push_back(new_boxes.second);
		}
		else {
			temporary_boxes->push_back(new_boxes.first);
			temporary_boxes->push_back(new_boxes.second);
		}
		break;
	}

	case 3:
		solution->push_back(box);
		break;
	default:
		break;
	}
}


//------------------------------------------------------------------------------------------
high_level_analysis::high_level_analysis(double& min_x, double& min_y, double& x_width, double& y_height) :
	low_level_fragmentation(min_x, min_y, x_width, y_height) {}

//------------------------------------------------------------------------------------------
high_level_analysis::high_level_analysis(Box& box) : low_level_fragmentation(box) {}

//------------------------------------------------------------------------------------------
void high_level_analysis::GetMinMax(const Box& box, min_max_vectors& min_max_vecs)
{
	std::vector<double> g_min;
	std::vector<double> g_max;

	double a1min, a2min, a1max, a2max;
	double xmin, xmax, ymin, ymax;

	box.GetParameters(xmin, ymin, xmax, ymax);

	xmax = xmin + xmax;
	ymax = ymin + ymax;

	double curr_box_diagonal = box.GetDiagonal();

	if (curr_box_diagonal <= g_precision)
	{
		g_min.push_back(0);
		g_max.push_back(0);

		min_max_vecs.first = g_min;
		min_max_vecs.second = g_max;

		return;
	}

	// MIN
	// ������� g1(x1,x2)
	a1min = __min(abs(xmin), abs(xmax));
	a2min = __min(abs(ymin), abs(ymax));
	g_min.push_back(g1(a1min, a2min));

	// ������� g2(x1,x2)
	a1min = __max(abs(xmin), abs(xmax));
	a2min = __max(abs(ymin), abs(ymax));
	g_min.push_back(g2(a1min, a2min));

	// ������� g3(x1,x2)
	a1min = __min(abs(xmin - g_l0), abs(xmax - g_l0));
	a2min = __min(abs(ymin), abs(ymax));
	g_min.push_back(g3(a1min, a2min));

	// ������� g4(x1,x2)
	a1min = __max(abs(xmin - g_l0), abs(xmax - g_l0));
	a2min = __max(abs(ymin), abs(ymax));
	g_min.push_back(g4(a1min, a2min));

	// MAX
	// ������� g1(x1,x2)
	a1max = __max(abs(xmin), abs(xmax));
	a2max = __max(abs(ymin), abs(ymax));
	g_max.push_back(g1(a1max, a2max));

	// ������� g2(x1,x2)
	a1max = __min(abs(xmin), abs(xmax));
	a2max = __min(abs(ymin), abs(ymax));
	g_max.push_back(g2(a1max, a2max));

	// ������� g3(x1,x2)
	a1max = __max(abs(xmin - g_l0), abs(xmax - g_l0));
	a2max = __max(abs(ymin), abs(ymax));
	g_max.push_back(g3(a1max, a2max));

	// ������� g4(x1,x2)
	a1max = __min(abs(xmin - g_l0), abs(xmax - g_l0));
	a2max = __min(abs(ymin), abs(ymax));
	g_max.push_back(g4(a1max, a2max));

	min_max_vecs.first = g_min;
	min_max_vecs.second = g_max;
}

// ������� GetSolution() ��������� �������� ���������� ������� ������� ������������
void high_level_analysis::GetSolution()
{
	current_box = Box(-g_l1_max, 0, g_l2_max + g_l0 + g_l1_max, __min(g_l1_max, g_l2_max));
	std::vector<Box> current_boxes;
	temporary_boxes->push_back(current_box);
	std::vector<Box> empty;

	int level = FindTreeDepth();

	for (int i = 0; i < (level + 1); ++i)
	{
		temporary_boxes.move_out(current_boxes);
		temporary_boxes.set_value(empty);
		cilk_for(int j = 0; j < current_boxes.size(); ++j)
			GetBoxType(current_boxes[j]);
	}
}

// ������� WriteResults() ���������� ��������� ���������� box-�� (����������� � �������� ������������, � ��������� ������� � �� ���������, �� ����������� ��������) � �������� ����� ��� ���������� ������������
void WriteResults(const char* file_names[])
{
	double x_min, y_min, width, height;
	std::vector <Box> temp;

	std::ofstream fsolution(file_names[0]);
	std::ofstream fboundary(file_names[1]);
	std::ofstream fnot_solution(file_names[2]);

	solution.move_out(temp);
	for (int i = 0; i < temp.size(); i++)
	{

		temp[i].GetParameters(x_min, y_min, width, height);
		fsolution << x_min << " " << y_min << " " << width << " " << height << '\n';
	}

	temp.clear();
	boundary.move_out(temp);
	for (int i = 0; i < temp.size(); i++)
	{
		temp[i].GetParameters(x_min, y_min, width, height);
		fboundary << x_min << " " << y_min << " " << width << " " << height << '\n';
	}

	temp.clear();
	not_solution.move_out(temp);
	for (int i = 0; i < temp.size(); i++)
	{
		temp[i].GetParameters(x_min, y_min, width, height);
		fnot_solution << x_min << " " << y_min << " " << width << " " << height << '\n';
	}

	fsolution.close();
	fboundary.close();
	fnot_solution.close();
}