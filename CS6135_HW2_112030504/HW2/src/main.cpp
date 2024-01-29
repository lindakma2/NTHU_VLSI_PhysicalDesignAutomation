#include<iostream>
#include<fstream>
#include<string>
#include<map>
#include<vector>
#include<algorithm>
#include<ctime>
#include<chrono>
#include <cstdlib> 
using namespace std;

class Cell {
public:
	int cell_id;
	int libcell_id;
	int pins = 0;
	vector<int> net_in_cell;
};
class Net {
public:
	int net_id;
	int A_set = 0;
	int B_set = 0;
	int cell_num = 0;
	vector<int> cell_in_net;
};
class LibCell {
public:
	int libcell_id;
	int width;
	int height;
	int area;
};
class Tech {
public:
	int tech_id;
	int libcell_num = 0;
	vector<LibCell>libcell_in_tech;
};

vector<string>split_string(string str) {
	vector<string> v;
	while (1) {
		v.push_back(str.substr(0, str.find(" ")));
		str = str.substr(str.find(" ") + 1, str.length());
		if (str.find(" ") == -1) {
			v.push_back(str);
			break;
		}
	}
	return v;
}
//int argc, char* argv[]
int main(int argc, char* argv[])
{
	//ifstream File_C("sample.txt"); //read net file
	//ofstream output("output.txt");
	ifstream File_C(argv[1]); //read net file
	ofstream output(argv[2]);
	//txtfile initial
	int NumTechs;
	int DieSize_width;
	int DieSize_height;
	int DieA;
	int DieB;
	int NumCells; //cell total number
	int NumNets; //net total number

	//varible to record 
	int DieA_tech; //record DieA tech type 
	int DieB_tech; //record DieB tech type
	long DieA_useful_area = 0; //count the useful area in DieA
	long DieB_useful_area = 0; //count the useful area in DieB

	//restore net cell libcell
	vector<Cell> cell_given; //index is id-1
	vector<Net> net_given;
	vector<Tech> tech_given;
	map<string, int> tech_group; //order tech numder to name <tech name,order number>
	map<int, int>cell_locked; //cell move and lock 0=>unlocked 1=>locked
	//read file from .txt
	string read_data; //read each line in file
	vector<string> v;
	string libcell_in_tech;
	int tech_temp = 0; //give each tech a number
	bool make_cell = true; //creat cell or record detail
	int net_temp = 0; //recent net id 
	int cell_temp = 0; //recent cell id
	while (getline(File_C, read_data)) {
		//let getline can keep reading although meet empty line
		if (read_data.length() == 0)
		{
			continue;
		}
		v = split_string(read_data);
		if (v[0] == "NumTechs")
		{
			NumTechs = stoi(v[1]);
		}
		else if (v[0] == "Tech")
		{
			Tech tech;
			tech.tech_id = tech_temp;
			tech.libcell_num = stoi(v[2]);
			tech_group[v[1]] = tech_temp;
			tech_given.push_back(tech);
			tech_temp++;
		}
		else if (v[0] == "LibCell")
		{
			LibCell libcell;
			libcell.libcell_id = stoi(v[1].substr(v[1].find("C") + 1, v[1].length()));
			libcell.width = stoi(v[2]);
			libcell.height = stoi(v[3]);
			libcell.area = stoi(v[2]) * stoi(v[3]);
			tech_given[tech_temp - 1].libcell_in_tech.push_back(libcell);
		}
		else if (v[0] == "DieSize")
		{
			DieSize_width = stoi(v[1]);
			DieSize_height = stoi(v[2]);
		}
		else if (v[0] == "DieA")
		{
			DieA = stoi(v[2]);
			DieA_tech = tech_group[v[1]];
			DieA_useful_area = (long)float(DieSize_width) * float(DieSize_height) * float(DieA) / 100;
		}
		else if (v[0] == "DieB")
		{
			DieB = stoi(v[2]);
			DieB_tech = tech_group[v[1]];
			DieB_useful_area = (long)float(DieSize_width) * float(DieSize_height) * float(DieB) / 100;
		}
		else if (v[0] == "NumCells")
		{
			NumCells = stoi(v[1]);
			//cout << "ok" << endl;
		}
		else if (v[0] == "Cell" && make_cell)
		{
			Cell cell;
			cell.cell_id = stoi(v[1].substr(v[1].find("C") + 1, v[1].length()));
			cell.libcell_id = stoi(v[2].substr(v[2].find("C") + 1, v[2].length()));
			cell_given.push_back(cell);
			cell_locked[cell.cell_id] = 0;
		}
		else if (v[0] == "NumNets")
		{
			NumNets = stoi(v[1]);
		}
		else if (v[0] == "Net")
		{
			make_cell = false;
			net_temp = stoi(v[1].substr(v[1].find("N") + 1, v[1].length()));
			Net net;
			net.net_id = stoi(v[1].substr(v[1].find("N") + 1, v[1].length()));
			net.cell_num = stoi(v[2]);
			net_given.push_back(net);
		}
		else if (v[0] == "Cell")
		{
			cell_temp = stoi(v[1].substr(v[1].find("C") + 1, v[1].length()));
			cell_given[cell_temp - 1].net_in_cell.push_back(net_temp);
			cell_given[cell_temp - 1].pins++;
			net_given[net_temp - 1].cell_in_net.push_back(cell_temp);
		}
		vector <string>().swap(v);
	}
	/*
	* make AB partioning
	*/
	bool flagA_full = false; //A is full or not 0=>have space 1=>full
	bool flagB_full = false; //B is full or not 0=>have space 1=>full
	int DieA_cellnum = 0;
	int DieB_cellnum = 0;
	//bool partition_finish = false; //still have cell didn't partition yet
	int cellid_to_partition; //cell is choose to partion
	int max_pin_num = -1;
	long dieA_totalarea = 0;
	long dieB_totalarea = 0;
	int die_full_check;
	map<int, char> partition_set; //record the cell id had already clustering<cell_id,group> A or B
	//partion from the first untill full
	//method 1
	for (int i = 0; i < NumCells; i++) {
		cellid_to_partition = cell_given[i].cell_id;
		if (!flagA_full)
		{
			//put cell in to DieA
			die_full_check = dieA_totalarea;
			die_full_check += tech_given[DieA_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;
			if (die_full_check > DieA_useful_area)
			{
				dieB_totalarea += tech_given[DieB_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;
				DieB_cellnum++;
				partition_set[cell_given[cellid_to_partition - 1].cell_id] = 'B';
				for (int j = 0; j < cell_given[cellid_to_partition - 1].net_in_cell.size(); j++)
				{
					net_given[(cell_given[cellid_to_partition - 1].net_in_cell[j]) - 1].B_set++;
				}
				if (max_pin_num < cell_given[cellid_to_partition - 1].pins)
				{
					max_pin_num = cell_given[cellid_to_partition - 1].pins;
				}
				if (dieB_totalarea >= DieB_useful_area) {
					flagB_full = true;
				}
			}
			else
			{
				dieA_totalarea += tech_given[DieA_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;;
				DieA_cellnum++;
				partition_set[cell_given[cellid_to_partition - 1].cell_id] = 'A';
				for (int j = 0; j < cell_given[cellid_to_partition - 1].net_in_cell.size(); j++)
				{
					net_given[(cell_given[cellid_to_partition - 1].net_in_cell[j]) - 1].A_set++;
				}
				if (max_pin_num < cell_given[cellid_to_partition - 1].pins)
				{
					max_pin_num = cell_given[cellid_to_partition - 1].pins;
				}
				if (dieA_totalarea >= DieA_useful_area) {
					flagA_full = true;
				}
			}
		}
		else if (!flagB_full)
		{
			die_full_check = dieB_totalarea;
			die_full_check += tech_given[DieB_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;
			if (die_full_check > DieB_useful_area)
			{
				flagB_full = true;
				dieA_totalarea += tech_given[DieA_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;
				DieA_cellnum++;
				partition_set[cell_given[cellid_to_partition - 1].cell_id] = 'A';
				for (int j = 0; j < cell_given[cellid_to_partition - 1].net_in_cell.size(); j++)
				{
					net_given[(cell_given[cellid_to_partition - 1].net_in_cell[j]) - 1].A_set++;
				}
				if (max_pin_num < cell_given[cellid_to_partition - 1].pins)
				{
					max_pin_num = cell_given[cellid_to_partition - 1].pins;
				}
				if (dieA_totalarea >= DieA_useful_area) {
					flagA_full = true;
				}
			}
			else
			{
				dieB_totalarea += tech_given[DieB_tech].libcell_in_tech[(cell_given[i].libcell_id) - 1].area;;
				DieB_cellnum++;
				partition_set[cell_given[cellid_to_partition - 1].cell_id] = 'B';
				for (int j = 0; j < cell_given[cellid_to_partition - 1].net_in_cell.size(); j++)
				{
					net_given[(cell_given[cellid_to_partition - 1].net_in_cell[j]) - 1].B_set++;
				}
				if (max_pin_num < cell_given[cellid_to_partition - 1].pins)
				{
					max_pin_num = cell_given[cellid_to_partition - 1].pins;
				}
				if (dieB_totalarea >= DieB_useful_area) {
					flagB_full = true;
				}
			}
		}
	}
	//cout << dieA_totalarea << " " << DieA_useful_area << endl;
	//cout << dieB_totalarea << " " << DieB_useful_area << endl;
	cout << "initial partition finish" << endl;
	for (auto it = partition_set.begin(); it != partition_set.end(); it++) {
		//cout << it->first << it->second << endl;
	}
	/*---------------intital cell gain-------------------*/
	int which_cell = -1;
	int F;
	int T;
	int cellid; //recored temp cell id
	int netid; //recored temp net id
	int cell_gain = 0; //recored temp cell gain
	int max_gain = -1 * NumNets; //record max cell gain in die
	int Gk_temp = 0; //this step get Gk
	double fm_start, fm_end;
	map<int, int>cellgain_before; //cell gain before move
	map<int, int>cellgain_after; //cell gain after move
	map<int, vector<int>>cellgain_bucketlist; //<cellgain,this cellgain's cellid>
	vector<int>step_maxgain; //each step max gain 
	vector<int>step_Gk; //each step Gk
	fm_start = clock();
	for (int i = (-1) * (max_pin_num+1); i < (max_pin_num + 1); i++)
	{
		cellgain_bucketlist[i];
	}
	//every cellid already in two die
	for (auto it = partition_set.begin(); it != partition_set.end(); it++)
	{
		cell_gain = 0;
		cellid = cell_given[(it->first) - 1].cell_id;
		if (it->second == 'A') {
			which_cell = 0;
		}
		else {
			which_cell = 1;
		}
		for (int j = 0; j < cell_given[(it->first) - 1].net_in_cell.size(); j++)
		{
			F = 0;
			T = 0;
			netid = cell_given[(it->first) - 1].net_in_cell[j];
			if (which_cell == 0) {
				F = net_given[netid - 1].A_set;
				T = net_given[netid - 1].B_set;
			}
			else if (which_cell == 1) {
				F = net_given[netid - 1].B_set;
				T = net_given[netid - 1].A_set;
			}
			if (F == 1) {
				cell_gain++;
			}
			if (T == 0) {
				cell_gain--;
			}
		}
		cellgain_bucketlist.find(cell_gain)->second.push_back(cellid);
		if (cell_gain > max_gain) max_gain = cell_gain;
		cellgain_before[cellid] = cell_gain;
	}
	/*-------------------FM start------------------------*/
	vector<map<int, char>>partion_result; //cell partion result in each group
	vector<vector<Net>>net_result;
	map<int, int>cell_which_to_update; //cell in base cell's net <cellid,cell new gain>
	vector<int> change_cellid; //record the cellid which to change
	vector<int> unlocked_maxgain_cellid; //get maxgain vector from bucket list
	vector<int>::iterator found;
	bool find_swap_cell = false; //keep find untill find out swap cell
	bool die_can_swap; //record the check result
	int unlocked_cellnum = NumCells; //unlocked cell num 
	int max_Gk = -1 * NumNets; //record the max Gk in each step
	int max_Gk_step = 0; //record the step of max Gk
	int count = 0;
	cellgain_after = cellgain_before;
	//repeat until Gk<=0 
	while (1)
	{
		find_swap_cell = false;
		cell_which_to_update.clear();
		max_gain = -1 * NumNets;
		//find unlocked cell maxgain
		for (auto it = cellgain_before.begin(); it != cellgain_before.end(); it++)
		{
			if (cell_locked[it->first] == 0) {
				if (cellgain_before[it->first] > max_gain) {
					max_gain = cellgain_before[it->first];
				}
			}
		}
		unlocked_maxgain_cellid = cellgain_bucketlist.find(max_gain)->second;
		//find out the cell to swap
		for (int check_already = 0; check_already < unlocked_cellnum; check_already++)
		{
			die_can_swap = true;
			cellid_to_partition = unlocked_maxgain_cellid.back();
			unlocked_maxgain_cellid.pop_back();
			//if cell is in A know check is moveable to B
			if (partition_set.find(cellid_to_partition)->second == 'A') {
				which_cell = 0;
				die_full_check = dieB_totalarea;
				die_full_check += tech_given[DieB_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
				if (die_full_check > DieB_useful_area) {
					die_can_swap = false;
				}
			}
			else {
				which_cell = 1;
				die_full_check = dieA_totalarea;
				die_full_check += tech_given[DieA_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
				if (die_full_check > DieA_useful_area) {
					die_can_swap = false;
				}
			}
			//find out cell can swap
			if (die_can_swap) {
				found = std::find(cellgain_bucketlist.find(max_gain)->second.begin(), cellgain_bucketlist.find(max_gain)->second.end(), cellid_to_partition);
				cellgain_bucketlist.find(max_gain)->second.erase(found);
				find_swap_cell = true;
				unlocked_cellnum--;
				cell_locked[cellid_to_partition] = 1;
				break;
			}
			else {
				if (unlocked_maxgain_cellid.size() == 0) {
					while (unlocked_maxgain_cellid.size() == 0) {
						max_gain--;
						unlocked_maxgain_cellid = cellgain_bucketlist.find(max_gain)->second;
					}
				}
			}
		}
		if (!find_swap_cell) {
			break;
		}
		//cout <<"cellid_to_partition " << cellid_to_partition << endl;
		/*----------------update cell gain---------------------*/
		for (int i = 0; i < cell_given[cellid_to_partition - 1].net_in_cell.size(); i++) {
			F = 0;
			T = 0;
			netid = cell_given[cellid_to_partition - 1].net_in_cell[i];
			for (int j = 0; j < net_given[netid - 1].cell_in_net.size(); j++) {
				cellid = net_given[netid - 1].cell_in_net[j];
				if (cell_which_to_update.find(cellid) == cell_which_to_update.end()) {
					cell_which_to_update[cellid] = 0;
				}
			}
			if (which_cell == 0) {
				F = net_given[netid - 1].A_set;
				T = net_given[netid - 1].B_set;
			}
			else if (which_cell == 1) {
				F = net_given[netid - 1].B_set;
				T = net_given[netid - 1].A_set;
			}
			/*-----------------before move------------------*/
			if (T == 0) {
				for (int j = 0; j < net_given[netid - 1].cell_in_net.size(); j++) {
					cellid = net_given[netid - 1].cell_in_net[j];
					cellgain_after[cellid]++;
					cell_which_to_update.find(cellid)->second++;
				}
			}
			else if (T == 1) {
				if (which_cell == 0) {
					for (int j = 0; j < net_given[netid - 1].cell_in_net.size(); j++) {
						cellid = net_given[netid - 1].cell_in_net[j];
						if (partition_set.find(cellid)->second == 'B') {
							cellgain_after[cellid]--;
							cell_which_to_update.find(cellid)->second--;
						}
					}
				}
				else {
					for (int j = 0; j < net_given[netid - 1].cell_in_net.size(); j++) {
						cellid = net_given[netid - 1].cell_in_net[j];
						if (partition_set.find(cellid)->second == 'A') {
							cellgain_after[cellid]--;
							cell_which_to_update.find(cellid)->second--;
						}
					}
				}
			}
			/*---------move cell influence gain----------*/
			T++;
			F--;
			if (which_cell == 0) {
				net_given[netid - 1].A_set--;
				net_given[netid - 1].B_set++;
			}
			else {
				net_given[netid - 1].A_set++;
				net_given[netid - 1].B_set--;
			}
			/*----------------after move----------------*/
			if (F == 0) {
				for (int j = 0; j < net_given[netid - 1].cell_in_net.size(); j++) {
					cellid = net_given[netid - 1].cell_in_net[j];
					cellgain_after[cellid]--;
					cell_which_to_update.find(cellid)->second--;
				}
			}
			else if (F == 1) {
				if (which_cell == 0) {
					if (partition_set.find(cellid)->second == 'A') {
						cellgain_after[cellid]++;
						cell_which_to_update.find(cellid)->second++;
					}
				}
				else {
					if (partition_set.find(cellid)->second == 'B') {
						cellgain_after[cellid]++;
						cell_which_to_update.find(cellid)->second++;
					}
				}
			}
		}
		/*----------------swap the cell-------------------*/
		//move cell
		if (which_cell == 0) //A->B
		{
			partition_set[cellid_to_partition] = 'B';
			dieA_totalarea -= tech_given[DieA_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
			dieB_totalarea += tech_given[DieB_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
		}
		else //B->A
		{
			partition_set[cellid_to_partition] = 'A';
			dieA_totalarea += tech_given[DieA_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
			dieB_totalarea -= tech_given[DieB_tech].libcell_in_tech[(cell_given[cellid_to_partition - 1].libcell_id) - 1].area;
		}
		/*-------count Gk record prationting-------*/
		change_cellid.push_back(cellid_to_partition);
		step_maxgain.push_back(max_gain);
		Gk_temp += max_gain;
		if (Gk_temp > max_Gk) {
			max_Gk = Gk_temp;
			max_Gk_step = step_Gk.size();
		}
		step_Gk.push_back(Gk_temp);
		partion_result.push_back(partition_set);
		net_result.push_back(net_given);
		if (Gk_temp <= 0) {
			break;
		}
		fm_end = clock();
		if ((fm_end - fm_start) / CLOCKS_PER_SEC > 255) {
			break;
		}
		for (auto it = cell_which_to_update.begin(); it != cell_which_to_update.end(); it++) {
			cellid = it->first;
			if ((it->second != 0) && (cell_locked.find(cellid)->second != 1)) {
				found = std::find(cellgain_bucketlist.find(cellgain_before[cellid])->second.begin(), cellgain_bucketlist.find(cellgain_before[cellid])->second.end(), cellid);
				cellgain_bucketlist.find(cellgain_before[cellid])->second.erase(found);
				cellgain_bucketlist.find(cellgain_after[cellid])->second.push_back(cellid);
			}
		}
		cellgain_before = cellgain_after;
	}
	/*--------------------count cut size------------------------*/
	int cut_size = 0;
	int dieA_cellnum = 0;
	int dieB_cellnum = 0;
	vector<int>dieA_result;
	vector<int>dieB_result;
	//cout << partion_result.size() << endl;
	for (auto it = partion_result[max_Gk_step].begin(); it != partion_result[max_Gk_step].end(); it++) {
		if (it->second == 'A') {
			dieA_result.push_back(it->first);
			dieA_cellnum++;
		}
		if (it->second == 'B') {
			dieB_result.push_back(it->first);
			dieB_cellnum++;
		}
	}
	for (int i = 0; i < NumNets; i++) {
		if (net_result[max_Gk_step][i].A_set != 0 && net_result[max_Gk_step][i].B_set != 0) {
			cut_size++;
		}
	}
	output << "CutSize " << cut_size << endl;
	output << "DieA " << dieA_cellnum << endl;
	for (int i = 0; i < dieA_cellnum; i++) {
		output << "C" << dieA_result[i] << endl;
	}
	output << "DieB " << dieB_cellnum << endl;
	for (int i = 0; i < dieB_cellnum; i++) {
		output << "C" << dieB_result[i] << endl;
	}
	return 0;
}
