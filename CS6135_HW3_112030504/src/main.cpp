#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<map>
#include<ctime>
#include<math.h>
#include<stdlib.h>
#include<chrono>
using namespace std;

// hardblocks
class Block {

public:
	string name;
	int width;
	int height;
	int x;
	int y;
	float R; //aspect ratio
	int minarea = -1;
	Block* parent = NULL;
	Block* l_child = NULL;
	Block* r_child = NULL;
	bool choose = false;
	int position = -1; //1up 2down 3left 4right
};

// nets
class Net {

public:
	vector<int>module_id;
	int weight;
	int hpwl;
};

//variable in file
int Chip_width;   //chip width
int Chip_height;   //chip height
int NumSoftModules;
int NumFixedModules;
int NumNets;

//variable to record block and net
vector<Block>block_given;
vector<Net>net_given;
map<string, int>block_mapid; //front is softmodule, back is fixedmodule

//veriable for SA
vector<Block>initial_btree;
int initial_weight;
vector<Block>local_btree;
Block* local_root = NULL;
int local_weight;
vector<Block>best_btree;
Block* best_root = NULL;
int best_weight;
int xboundary_max;
int yboundary_max;
bool SA_success = true;

//initial floorplan
int tmp_x = 0;
bool insert_complete;
int insert_blockid;
int outof_btreeid=-1;

//O tree
Block* root = NULL;
vector<int>horzonital_contour;
vector<int>else_contour;
int overlap_fixedid; //record next plate to plan  n \  module     U   y  

//count time
auto t1 = chrono::steady_clock::now();
auto t2 = chrono::steady_clock::now();
auto sa_t1 = chrono::steady_clock::now();
auto sa_t2 = chrono::steady_clock::now();

//other usage
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
void sort_area() {
	Block temp;
	for (int i = 0; i < NumSoftModules; i++) {
		for (int j = 0; j < i; j++) {
			if (block_given[i].minarea > block_given[j].minarea) {
				temp = block_given[i];
				block_given[i] = block_given[j];
				block_given[j] = temp;
			}
		}
	}
}
void Preorder(Block* block) {  // e ǰl  
	if (block != NULL) {
		cout << "block x " << block->x << endl;
		cout << "block y " << block->y << endl;
		cout << "block id " << block->name << endl;
		cout << "block width " << block->width << endl;
		cout << "block height " << block->height << endl;
		Block* tmp = block->parent;
		if (tmp != NULL) {
			cout << "block parent" << tmp->name << endl<<endl;
		}
		Preorder(block->l_child);    //   j   l  
		Preorder(block->r_child);  //   j k l  
	}
}
int total_weight(int mode) {
	int Total_Weight = 0;
	if (mode == 1) {
		for (int i = 0; i < NumNets; i++) {
			int module1_id = net_given[i].module_id[0];
			int module2_id = net_given[i].module_id[1];
			int mouule1_centerx = (block_given[module1_id].x + block_given[module1_id].x + block_given[module1_id].width) / 2;
			int mouule1_centery = (block_given[module1_id].y + block_given[module1_id].y + block_given[module1_id].height) / 2;
			int mouule2_centerx = (block_given[module2_id].x + block_given[module2_id].x + block_given[module2_id].width) / 2;
			int mouule2_centery = (block_given[module2_id].y + block_given[module2_id].y + block_given[module2_id].height) / 2;
			Total_Weight += net_given[i].weight * (abs(mouule1_centerx - mouule2_centerx) + abs(mouule1_centery - mouule2_centery));
		}
	}
	else if (mode == 2) {
		for (int i = 0; i < NumNets; i++) {
			int module1_id = net_given[i].module_id[0];
			int module2_id = net_given[i].module_id[1];
			int mouule1_centerx = (local_btree[module1_id].x + local_btree[module1_id].x + local_btree[module1_id].width) / 2;
			int mouule1_centery = (local_btree[module1_id].y + local_btree[module1_id].y + local_btree[module1_id].height) / 2;
			int mouule2_centerx = (local_btree[module2_id].x + local_btree[module2_id].x + local_btree[module2_id].width) / 2;
			int mouule2_centery = (local_btree[module2_id].y + local_btree[module2_id].y + local_btree[module2_id].height) / 2;
			Total_Weight += net_given[i].weight * (abs(mouule1_centerx - mouule2_centerx) + abs(mouule1_centery - mouule2_centery));
		}
	}
	else if (mode == 3) {
		for (int i = 0; i < NumNets; i++) {
			int module1_id = net_given[i].module_id[0];
			int module2_id = net_given[i].module_id[1];
			int mouule1_centerx = (best_btree[module1_id].x + best_btree[module1_id].x + best_btree[module1_id].width) / 2;
			int mouule1_centery = (best_btree[module1_id].y + best_btree[module1_id].y + best_btree[module1_id].height) / 2;
			int mouule2_centerx = (best_btree[module2_id].x + best_btree[module2_id].x + best_btree[module2_id].width) / 2;
			int mouule2_centery = (best_btree[module2_id].y + best_btree[module2_id].y + best_btree[module2_id].height) / 2;
			Total_Weight += net_given[i].weight * (abs(mouule1_centerx - mouule2_centerx) + abs(mouule1_centery - mouule2_centery));
		}
	}
	return Total_Weight;
}
void store_local() {
	Block temp;
	local_btree.assign(block_given.size(), temp);
	for (int i = 0; i < block_given.size(); ++i) {
		local_btree[i]= block_given[i];
		if (local_btree[i].parent != NULL) {
			local_btree[i].parent = &local_btree[block_mapid[block_given[i].parent->name]];
		}
		if (local_btree[i].l_child != NULL) {
			local_btree[i].l_child = &local_btree[block_mapid[block_given[i].l_child->name]];
		}
		if (local_btree[i].r_child != NULL) {
			local_btree[i].r_child = &local_btree[block_mapid[block_given[i].r_child->name]];
		}
		if (&block_given[i] == root) {
			local_root = &local_btree[i];
		}
	}
	local_weight = total_weight(2);
	return;
}
void restore_local() {
	Block temp;
	for (int i = 0; i < block_given.size(); ++i) {
		//local_btree[i] = block_given[i];
		block_given[i].x = local_btree[i].x;
		block_given[i].y = local_btree[i].y;
		block_given[i].height = local_btree[i].height;
		block_given[i].width = local_btree[i].width;
		if (local_btree[i].parent != NULL) {
			block_given[i].parent = &block_given[block_mapid[local_btree[i].parent->name]];
		}
		else {
			block_given[i].parent = NULL;
		}
		if (local_btree[i].l_child != NULL) {
			block_given[i].l_child = &block_given[block_mapid[local_btree[i].l_child->name]];
		}
		else {
			block_given[i].l_child = NULL;
		}
		if (local_btree[i].r_child != NULL) {
			block_given[i].r_child = &block_given[block_mapid[local_btree[i].r_child->name]];
		}
		else {
			block_given[i].r_child = NULL;
		}
		if (&local_btree[i] == local_root) {
			root = &block_given[i];
		}
	}
	return;
}
void store_best() {
	//cout << "---------------store best" << endl;
	Block temp;
	best_btree.assign(local_btree.size(), temp);
	for (int i = 0; i < local_btree.size(); ++i) {
		best_btree[i] = local_btree[i];
		if (best_btree[i].parent != NULL) {
			best_btree[i].parent = &best_btree[block_mapid[local_btree[i].parent->name]];
		}
		if (best_btree[i].l_child != NULL) {
			best_btree[i].l_child = &best_btree[block_mapid[local_btree[i].l_child->name]];
		}
		if (best_btree[i].r_child != NULL) {
			best_btree[i].r_child = &best_btree[block_mapid[local_btree[i].r_child->name]];
		}
		if (&local_btree[i] == local_root) {
			best_root = &best_btree[i];
		}
	}
	best_weight = total_weight(3);
	//cout << "best_weight " << best_weight << endl;
	return;
}

//floorplam
void update_fixedy_contour(int updatefixed) {
	for (int i = block_given[updatefixed].x; i != block_given[updatefixed].x + block_given[updatefixed].width; ++i) {
		horzonital_contour[i] = block_given[updatefixed].y + block_given[updatefixed].height;
	}
}
int find_fixed(int x, int width, int y, int height) { //input the right bound of the range 
	int x_boundary = x + width;
	int y_boundary = y + height;
	overlap_fixedid = -1;
	for (int i = NumSoftModules; i < block_given.size(); i++) {
		if (block_given[i].y < y_boundary && block_given[i].y + block_given[i].height > y) {
			if (block_given[i].x < x_boundary && block_given[i].x + block_given[i].width>x) {
				if ((block_given[i].position >= 6 || block_given[i].position != 1) && !block_given[i].choose) {
					block_given[i].choose = true;
					update_fixedy_contour(i);
					//cout << "update contour " << block_given[i].name << endl;
				}
				if (block_given[i].position == 1 || block_given[i].position == 4 || block_given[i].position == 5 || block_given[i].position == 6 || block_given[i].position == 8) {
					overlap_fixedid = i;
					//cout << "update overlap_fixedid " << block_given[i].name << endl;
				}
				if (block_given[i].position == 3 || block_given[i].position == 4 || block_given[i].position == 7 || block_given[i].position == 8) {
					tmp_x = block_given[i].x + block_given[i].width;
					//cout << "update tmp_x " << block_given[i].name << endl;
				}

			}
		}
	}
	return overlap_fixedid;
}
int update_else_contour(int blockid, int x, int width, int y, int height) {
	//cout << "else_contour" << blockid << " " << x << " " << width << " " << height << endl;
	else_contour.clear();
	else_contour.assign(Chip_width * 2, 0);
	for (int i = 0; i < blockid; i++) {
		for (int j = block_given[i].x; j != block_given[i].x + block_given[i].width; ++j) {
			if (else_contour[j] < block_given[i].y + block_given[i].height) {
				else_contour[j] = block_given[i].y + block_given[i].height;
			}
		}
	}
	int boundary = x + width;
	int max = 0;
	//find max y_coordinate
	for (int i = x; i != boundary; ++i) {
		if (else_contour[i] > max) {
			max = else_contour[i];
		}
	}
	if (max > y) {
		return max;
	}
	else {
		return y;
	}
}
int update_contour(int x, int width, int height) {
	int boundary = x + width;
	int max = 0;
	//find max y_coordinate
	for (int i = x; i != boundary; ++i) {
		if (horzonital_contour[i] > max) {
			max = horzonital_contour[i];
		}
	}
	//update y_coordinate
	int max_horizon = max + height;
	for (int i = x; i != boundary; ++i) {
		horzonital_contour[i] = max_horizon;
	}
	return max;
}
int find_else_top(int x, int width) {
	int minheight = Chip_height;
	for (int i = 0; i < block_given.size(); i++) {
		if (block_given[i].x + block_given[i].width > x && block_given[i].x < width && block_given[i].y < minheight) {
			minheight = block_given[i].y;
		}
	}
	return minheight;
}
int find_else_right_withoutfixed(int y, int height, int x) {
	int minwidth = Chip_width;
	for (int i = 0; i < NumSoftModules; i++) {
		if (block_given[i].y + block_given[i].height > y && block_given[i].y < y + height && block_given[i].x < minwidth && block_given[i].x >= x) {
			minwidth = block_given[i].x;
		}
	}
	//cout << "find_else_right" << y << " " << height<<" "<< minwidth<<" "<<x << endl;
	return minwidth;
}
int find_else_top_withoutfixed(int x, int width, int y) {
	int minheight = Chip_height;
	for (int i = 0; i < NumSoftModules; i++) {
		if (block_given[i].x + block_given[i].width > x && block_given[i].x < width && block_given[i].y < minheight && block_given[i].y >= y) {
			minheight = block_given[i].y;
		}
	}
	//cout << "find_else_top" << x << " " << width << " " << minheight << endl;
	return minheight;
}
void reshape_insert(Block* block) {
	int max_boundary = 0;
	int min_boundary = 0;
	int max_boundarx = 0;
	int min_boundarx = 0;
	int reshape_width = 0;
	int reshape_height = 0;
	int available_area = 0;
	//cout << "reshape_insert name" << block->name << "insert block name" << block_given[insert_blockid].name << endl;

	if (block->l_child == NULL && !block_given[insert_blockid].choose) {
		if (find_fixed(block->x + block->width, Chip_width - (block->x + block->width), block->y, block->height) != -1) {
			max_boundarx = block_given[overlap_fixedid].x;
			min_boundarx = block->x + block->width;
			min_boundary = update_else_contour(insert_blockid, min_boundarx, block->width, block->y, block->height);
			max_boundary = find_else_top_withoutfixed(block->x + block->width, max_boundarx, min_boundary);
			//cout << "first-1" << endl;
			if (min_boundarx == max_boundarx) {
				goto try_upspace;
			}
			available_area = (max_boundarx - min_boundarx) * (max_boundary - min_boundary);
			reshape_width = (max_boundarx - min_boundarx);
			reshape_height = (block_given[insert_blockid].minarea / reshape_width) + 1;
			//cout << max_boundarx << " " << min_boundarx << " " << max_boundary << " " << min_boundary << endl;
			if (reshape_height * reshape_width <= available_area && find_fixed(min_boundarx, reshape_width, min_boundary, reshape_height) == -1) {
				block_given[insert_blockid].choose = true;
				block_given[insert_blockid].height = reshape_height;
				block_given[insert_blockid].width = reshape_width;
				block_given[insert_blockid].x = min_boundarx;
				block_given[insert_blockid].y = min_boundary;
				block_given[insert_blockid].parent = block;
				block->l_child = &block_given[insert_blockid];
				//cout << "reshape_insert first-1 finish planing " << block_given[insert_blockid].name << " " << block_given[insert_blockid].x << " " << block_given[insert_blockid].y << " " << block_given[insert_blockid].width << " " << block_given[insert_blockid].height << endl;
			}
			else {
				//cout << "insert left" << endl;
				max_boundarx = Chip_width;
				min_boundarx = block->x + block->width;
				min_boundary = block_given[overlap_fixedid].y + block_given[overlap_fixedid].height;
				max_boundary = find_else_top_withoutfixed(block->x + block->width, max_boundarx, min_boundary);
				available_area = (max_boundarx - min_boundarx) * (max_boundary - min_boundary);
				reshape_width = (max_boundarx - min_boundarx);
				reshape_height = (block_given[insert_blockid].minarea / reshape_width) + 1;
				//cout << max_boundarx << " " << min_boundarx << " " << max_boundary << " " << min_boundary << endl;
				if (reshape_height * reshape_width <= available_area) {
					block_given[insert_blockid].choose = true;
					block_given[insert_blockid].height = reshape_height;
					block_given[insert_blockid].width = reshape_width;
					block_given[insert_blockid].x = min_boundarx;
					block_given[insert_blockid].y = min_boundary;
					block_given[insert_blockid].parent = block;
					block->l_child = &block_given[insert_blockid];
					//cout << "reshape_insert first-2 finish planing " << block_given[insert_blockid].name << " " << block_given[insert_blockid].x << " " << block_given[insert_blockid].y << " " << block_given[insert_blockid].width << " " << block_given[insert_blockid].height << endl;
				}
			}
		}
		else {
			min_boundarx = block->x + block->width;
			max_boundarx = find_else_right_withoutfixed(block->y, Chip_height, min_boundarx);
			min_boundary = block->y;
			max_boundary = find_else_top_withoutfixed(block->x + block->width, Chip_width, min_boundary);
			if (min_boundarx == max_boundarx) {
				goto try_upspace;
			}
			available_area = (max_boundarx - min_boundarx) * (max_boundary - min_boundary);
			reshape_width = (max_boundarx - min_boundarx);
			reshape_height = (block_given[insert_blockid].minarea / reshape_width) + 1;
			//cout << max_boundarx << " " << min_boundarx << " " << max_boundary << " " << min_boundary << endl;
			if (reshape_height * reshape_width <= available_area) {
				block_given[insert_blockid].choose = true;
				block_given[insert_blockid].height = reshape_height;
				block_given[insert_blockid].width = reshape_width;
				block_given[insert_blockid].x = min_boundarx;
				block_given[insert_blockid].y = min_boundary;
				block_given[insert_blockid].parent = block;
				block->l_child = &block_given[insert_blockid];
				//cout << "reshape_insert first-3 finish planing " << block_given[insert_blockid].name << " " << block_given[insert_blockid].x << " " << block_given[insert_blockid].y << " " << block_given[insert_blockid].width << " " << block_given[insert_blockid].height << endl;
			}
		}
	}
	else if (block->r_child == NULL && !block_given[insert_blockid].choose) {
	try_upspace:
		if (find_fixed(block->x, block->width, block->y + block->height, block->height) != -1) {
			max_boundarx = block->x + block->width;
			min_boundarx = block->x;
			min_boundary = update_else_contour(insert_blockid, min_boundarx, block->width, block->y + block->height, block->height);
			max_boundary = block_given[find_fixed(block->x, block->width, block->y + block->height, block->height)].y;
			//cout << "second-1" << endl;
			if (min_boundary == max_boundary) {
				goto full_of_chip;
			}
			available_area = (max_boundarx - min_boundarx) * (max_boundary - min_boundary);
			reshape_height = (max_boundary - min_boundary);
			reshape_width = (block_given[insert_blockid].minarea / reshape_height) + 1;
			//cout << max_boundarx << " " << min_boundarx << " " << max_boundary << " " << min_boundary << endl;
			if (reshape_height * reshape_width <= available_area) {
				block_given[insert_blockid].choose = true;
				block_given[insert_blockid].height = reshape_height;
				block_given[insert_blockid].width = reshape_width;
				block_given[insert_blockid].x = min_boundarx;
				block_given[insert_blockid].y = min_boundary;
				block_given[insert_blockid].parent = block;
				block->r_child = &block_given[insert_blockid];
				//cout << "reshape_insert second-1 finish planing " << block_given[insert_blockid].name << " " << block_given[insert_blockid].x << " " << block_given[insert_blockid].y << " " << block_given[insert_blockid].width << " " << block_given[insert_blockid].height << endl;
			}
		}
		else {
			//cout << "up-space" << endl;
			max_boundarx = block->x + block->width;
			min_boundarx = block->x;
			min_boundary = block->y + block->height;
			max_boundary = find_else_top_withoutfixed(min_boundarx, max_boundarx, min_boundary);
			if (min_boundary == max_boundary) {
				goto full_of_chip;
			}
			available_area = (max_boundarx - min_boundarx) * (max_boundary - min_boundary);
			reshape_width = (max_boundarx - min_boundarx);
			reshape_height = (block_given[insert_blockid].minarea / reshape_width) + 1;
			//cout << max_boundarx << " " << min_boundarx << " " << max_boundary << " " << min_boundary << endl;
			if (min_boundarx + reshape_width != Chip_width) {
				reshape_height = (max_boundary - min_boundary);
				reshape_width = (block_given[insert_blockid].minarea / reshape_height) + 1;
			}
			if (reshape_height * reshape_width <= available_area&& (float)reshape_width/ (float)reshape_height<2) {
				block_given[insert_blockid].choose = true;
				block_given[insert_blockid].height = reshape_height;
				block_given[insert_blockid].width = reshape_width;
				block_given[insert_blockid].x = min_boundarx;
				block_given[insert_blockid].y = min_boundary;
				block_given[insert_blockid].parent = block;
				block->r_child = &block_given[insert_blockid];
				//cout << "reshape_insert second-2 finish planing " << block_given[insert_blockid].name << " " << block_given[insert_blockid].x << " " << block_given[insert_blockid].y << " " << block_given[insert_blockid].width << " " << block_given[insert_blockid].height << endl;
			}
		}
	}

full_of_chip:
	if (block->l_child != NULL && block->r_child != NULL) {
		reshape_insert(block->l_child);  //   j   l  
		reshape_insert(block->r_child);  //   j k l  
	}
	else if (block->l_child != NULL) {
		reshape_insert(block->l_child);  //   j   l  
	}
	else if (block->r_child != NULL) {
		reshape_insert(block->r_child);  //   j k l  
	}
	return;
}
void initial_floorplan() {
	horzonital_contour.clear();
	horzonital_contour.assign(Chip_width * 2, 0);
	//initial B* tree
	int count = NumNets;
	Block* cur_rblock = NULL;
	Block* last_id = NULL;
	int block_id;
	for (int i = 0; i < NumSoftModules; i++) {
		block_id = i;
		//cout << "block_id " << block_id << endl;
	replace:
		//make the tree from all block is define initial
		if (root == NULL) {
			if (find_fixed(tmp_x, block_given[block_id].width, horzonital_contour[tmp_x], horzonital_contour[tmp_x] + block_given[block_id].height) == -1) {
				root = &block_given[block_id];
				root->choose = true;
				root->x = tmp_x;
				root->y = update_contour(tmp_x, root->width, root->height);
				tmp_x += root->width;
				cur_rblock = root;
				last_id = root;
				//cout << "root finish planing " << block_given[block_id].name << " " << block_given[block_id].x << " " << block_given[block_id].y << endl;
			}
			else {
				//cout << "wrong" << endl;
			}
		}
		else if ((tmp_x + block_given[block_id].width) <= Chip_width && (horzonital_contour[tmp_x] + block_given[block_id].height) <= Chip_height) {
			//fixed overlap on right
			//cout << "first " << block_given[block_id].name << endl;
			//cout << tmp_x << " " << block_given[block_id].width << " " << horzonital_contour[tmp_x] << " " << block_given[block_id].height << endl;
			if (find_fixed(tmp_x, block_given[block_id].width, horzonital_contour[tmp_x], block_given[block_id].height) != -1) {
				//cout << "find region 4" << overlap_fixedid << endl;
				tmp_x = block_given[overlap_fixedid].x + block_given[overlap_fixedid].width;
				goto replace;
			}
			else {
				block_given[block_id].choose = true;
				last_id->l_child = &block_given[block_id];
				block_given[block_id].parent = last_id;
				last_id = last_id->l_child;
				block_given[block_id].x = tmp_x;
				block_given[block_id].y = update_contour(tmp_x, block_given[block_id].width, block_given[block_id].height);
				tmp_x += block_given[block_id].width;
				//cout << "first finish planing " << block_given[block_id].name << " " << block_given[block_id].x << " " << block_given[block_id].y << endl;

			}
		}
		else if ((horzonital_contour[tmp_x] + block_given[block_id].height) <= Chip_height) {
			//fixed overlap on top
			//cout << "second" << endl;
			tmp_x = 0;
			//cout << block_given[block_id].name << " @" << tmp_x << " " << block_given[block_id].width << " " << horzonital_contour[tmp_x] << " " << block_given[block_id].height << endl;
			if (find_fixed(tmp_x, block_given[block_id].width, horzonital_contour[tmp_x], block_given[block_id].height) != -1) {
				//cout << "--------------find----------------" << overlap_fixedid << endl;
				goto find_place_insert;
			}
			else {
				for (int k = NumSoftModules; k < block_given.size(); k++) {
					if (block_given[k].y + block_given[k].height == horzonital_contour[tmp_x] && block_given[k].x == 0) {
						//cout << "wired else " << block_given[k].name << endl;
						tmp_x = block_given[k].x + block_given[k].width;
						if ((horzonital_contour[tmp_x] + block_given[block_id].height) > Chip_height) {
							goto find_place_insert;
						}
						break;
					}
				}
				for (int k = 0; k < i; k++) {
					if (block_given[k].x == tmp_x&& block_given[k].y+ block_given[k].height== horzonital_contour[tmp_x]) {
						cur_rblock = &block_given[k];
						break;
					}
				}
				//cout << tmp_x << " " << block_given[block_id].width << " " << horzonital_contour[tmp_x] << " " << block_given[block_id].height << endl;
				block_given[block_id].choose = true;
				cur_rblock->r_child = &block_given[block_id];
				block_given[block_id].parent = cur_rblock;
				cur_rblock = &block_given[block_id];
				last_id = &block_given[block_id];
				block_given[block_id].x = tmp_x;
				block_given[block_id].y = update_contour(tmp_x, block_given[block_id].width, block_given[block_id].height);
				tmp_x += block_given[block_id].width;
				//cout << "second finish planing " << block_given[block_id].name << " " << block_given[block_id].x << " " << block_given[block_id].y << endl;
			}
		}
		else {
		find_place_insert:
			//cout << "else" << endl;
			Block* temp;
			Block* r_temp;
			temp = root;
			int tmp_y = 0;
			//find right child position to insert
			//if each row final left child have space to put right child
			r_temp = root;
			while (r_temp->r_child != NULL) {
				temp = r_temp;
				while (temp->l_child != NULL) {
					temp = temp->l_child;
				}
				tmp_y = temp->y;
			pass_fixed_retry:
				//cout << "tmp_y" << tmp_y << endl;
				if ((temp->x + temp->width + block_given[block_id].width) <= Chip_width && (tmp_y + block_given[block_id].height) <= Chip_height) {
					//fixed overlap on right
					tmp_x = temp->x + temp->width;
					//cout << "first " << block_given[block_id].name << endl;
					//cout << tmp_x << " " << block_given[block_id].width << " " << tmp_y << " " << block_given[block_id].height << endl;
					if (find_fixed(tmp_x, block_given[block_id].width, tmp_y, block_given[block_id].height) != -1) {
						//cout << "find" << overlap_fixedid << endl;
						tmp_y = block_given[overlap_fixedid].y + block_given[overlap_fixedid].height;
						goto pass_fixed_retry;
					}
					else if (find_else_top(tmp_x, tmp_x + block_given[block_id].width) < tmp_y + block_given[block_id].height) {
						//cout << "keep" << endl;
						r_temp = r_temp->r_child;
						continue;
					}
					else {
						block_given[block_id].choose = true;
						temp->l_child = &block_given[block_id];
						block_given[block_id].parent = temp;
						cur_rblock = &block_given[block_id];
						last_id = &block_given[block_id];
						block_given[block_id].x = temp->x + temp->width;
						block_given[block_id].y = tmp_y;
						//update_else_contour(block_given[block_id].x, block_given[block_id].width, block_given[block_id].y, block_given[block_id].height);
						tmp_x = temp->x + temp->width + block_given[block_id].width;
						//cout << "else first finish planing " << block_given[block_id].name << " " << block_given[block_id].x << " " << block_given[block_id].y << endl;
						break;
					}
				}
				r_temp = r_temp->r_child;
			}
			//find the final row have space to put right child
			if (!block_given[block_id].choose) {
				//cout << "final row to insert" << endl;
				//cout << r_temp->name << endl;
				while (r_temp->l_child != NULL) {
					tmp_x = r_temp->x;
					//cout << tmp_x << " " << block_given[block_id].width << " " << horzonital_contour[tmp_x] << " " << block_given[block_id].height << endl;
					if ((r_temp->y + r_temp->height + block_given[block_id].height) <= Chip_height && r_temp->r_child == NULL) {
						//fixed overlap on top
						if (find_fixed(tmp_x, block_given[block_id].width, r_temp->y + r_temp->height, block_given[block_id].height) == -1) {
							block_given[block_id].choose = true;
							r_temp->r_child = &block_given[block_id];
							block_given[block_id].parent = r_temp;
							cur_rblock = &block_given[block_id];
							last_id = &block_given[block_id];
							block_given[block_id].x = tmp_x;
							block_given[block_id].y = r_temp->y + r_temp->height;
							tmp_x = block_given[block_id].width + block_given[block_id].x;
							//cout << "else second finish planing " << block_given[block_id].name << " " << block_given[block_id].x << " " << block_given[block_id].y << endl;
							break;
						}
					}
					r_temp = r_temp->l_child;
				}
			}
		}
		if (!block_given[block_id].choose) {
			//cout << "-----------------find place to insert----------------" << endl;
			insert_blockid = block_id;
			reshape_insert(root);
			int location_maxy = 0;
			int location_miny = 0;
			if (!block_given[insert_blockid].choose) {
				block_given[block_id].choose = true;
				outof_btreeid = block_id;
				for (int a = NumSoftModules; a < block_given.size(); a++) {
					if (block_given[a].x == 0) {
						if (block_given[a].y + block_given[a].height == Chip_height) {
							location_maxy = block_given[a].y;
						}
						if (block_given[a].y + block_given[a].height > location_miny && block_given[a].y + block_given[a].height != Chip_height) {
							location_miny = block_given[a].y + block_given[a].height;
						}
					}
				}
				block_given[block_id].x = 0;
				block_given[block_id].y = location_miny;
				int reshape_height = (location_maxy - location_miny);
				int reshape_width = (block_given[block_id].minarea / reshape_height) + 1;
				block_given[block_id].height = reshape_height;
				block_given[block_id].width = reshape_width;
			}
		}
	}

	return;
}

//SA
int local_count = 0;
void rotate(int candidate) {
	//cout << local_btree[candidate].name << endl;
	if (candidate == outof_btreeid) {
		return;
	}
	local_btree[candidate].R = 1 / local_btree[candidate].R;
	int tmp = local_btree[candidate].width;
	local_btree[candidate].width = local_btree[candidate].height;
	local_btree[candidate].height = tmp;
	//cout << local_btree[candidate].width << " " << local_btree[candidate].height << endl;
	return;
}
void swap(int candidate1, int candidate2) {
	if (candidate1 == outof_btreeid|| candidate2 == outof_btreeid) {
		return;
	}
	//cout << "swap " << local_btree[candidate1].name << " " << local_btree[candidate2].name << endl;
	Block* candidate1_p = local_btree[candidate1].parent;
	Block* candidate2_p = local_btree[candidate2].parent;
	//update 1's parent child to 2
	if (candidate1_p == NULL) {
		local_root = &local_btree[candidate2];
	}
	else if (candidate1_p->l_child == &local_btree[candidate1]) {
		candidate1_p->l_child = &local_btree[candidate2];
	}
	else {
		candidate1_p->r_child = &local_btree[candidate2];
	}
	//update 2's parent child to 1
	if (candidate2_p == NULL) {
		local_root = &local_btree[candidate1];
	}
	else if (candidate2_p->l_child == &local_btree[candidate2]) {
		candidate2_p->l_child = &local_btree[candidate1];
	}
	else {
		candidate2_p->r_child = &local_btree[candidate1];
	}
	//update candidate left chiild
	Block* candidate1_l = local_btree[candidate1].l_child;
	Block* candidate2_l = local_btree[candidate2].l_child;

	if (candidate1_l != NULL) candidate1_l->parent = &local_btree[candidate2];
	if (candidate2_l != NULL) candidate2_l->parent = &local_btree[candidate1];

	Block* candidate1_r = local_btree[candidate1].r_child;
	Block* candidate2_r = local_btree[candidate2].r_child;
	if (candidate1_r != NULL) candidate1_r->parent = &local_btree[candidate2];
	if (candidate2_r != NULL) candidate2_r->parent = &local_btree[candidate1];

	//internal change
	Block* tmp_parent = local_btree[candidate1].parent;
	local_btree[candidate1].parent = local_btree[candidate2].parent;
	local_btree[candidate2].parent = tmp_parent;


	Block* tmp_lchild = local_btree[candidate1].l_child;
	local_btree[candidate1].l_child = local_btree[candidate2].l_child;
	local_btree[candidate2].l_child = tmp_lchild;

	Block* tmp_rchild = local_btree[candidate1].r_child;
	local_btree[candidate1].r_child = local_btree[candidate2].r_child;
	local_btree[candidate2].r_child = tmp_rchild;

	return;
}
void pc_swap(int candidate1, int candidate2) {
	if (candidate1 == outof_btreeid || candidate2 == outof_btreeid) {
		return;
	}
	if (local_btree[candidate2].parent == &local_btree[candidate1]) { //1 is 2's parent
		int tmp = candidate1;
		candidate1 = candidate2;
		candidate2 = tmp;
	}
	Block* a1_p = local_btree[candidate1].parent;
	Block* a1_l = local_btree[candidate1].l_child;
	Block* a1_r = local_btree[candidate1].r_child;
	Block* b2_p = local_btree[candidate2].parent;
	Block* b2_l = local_btree[candidate2].l_child;
	Block* b2_r = local_btree[candidate2].r_child;
	//cout << "pc_swap " << local_btree[candidate1].name << " " << local_btree[candidate2].name << endl;
	if (local_btree[candidate1].parent == &local_btree[candidate2]) { //2 is 1's parent
		if (&local_btree[candidate2] == local_root) {
			local_root = &local_btree[candidate1];
		}
		if (b2_p != NULL) {
			if (b2_p->l_child == &local_btree[candidate2]) {
				b2_p->l_child = &local_btree[candidate1];
			}
			else if (b2_p->r_child == &local_btree[candidate2]) {
				b2_p->r_child = &local_btree[candidate1];
			}
		}
		
		//internal change
		if (local_btree[candidate2].l_child == &local_btree[candidate1]) {
			//internal change
			local_btree[candidate1].parent = local_btree[candidate2].parent;
			local_btree[candidate2].parent = &local_btree[candidate1];

			local_btree[candidate2].l_child = local_btree[candidate1].l_child;
			local_btree[candidate1].l_child = &local_btree[candidate2];

			Block* tmp_rchild = local_btree[candidate1].r_child;
			local_btree[candidate1].r_child = local_btree[candidate2].r_child;
			local_btree[candidate2].r_child = tmp_rchild;
			if (a1_l != NULL) {
				a1_l->parent = &local_btree[candidate2];
			}
			if (a1_r != NULL) {
				a1_r->parent = &local_btree[candidate2];
			}
			if (b2_r != NULL) {
				b2_r->parent = &local_btree[candidate1];
			}
		}
		else if(local_btree[candidate2].r_child == &local_btree[candidate1]){
			local_btree[candidate1].parent = local_btree[candidate2].parent;
			local_btree[candidate2].parent = &local_btree[candidate1];

			Block* tmp_lchild = local_btree[candidate1].l_child;
			local_btree[candidate1].l_child = local_btree[candidate2].l_child;
			local_btree[candidate2].l_child = tmp_lchild;

			local_btree[candidate2].r_child = local_btree[candidate1].r_child;
			local_btree[candidate1].r_child = &local_btree[candidate2];
			if (a1_l != NULL) {
				a1_l->parent = &local_btree[candidate2];
			}
			if (b2_l != NULL) {
				b2_l->parent = &local_btree[candidate1];
			}
			if (a1_r != NULL) {
				a1_r->parent = &local_btree[candidate2];
			}
		}
		
	}
	return;
}
void move(int candidate1, int candidate2) {
	if (candidate1 == outof_btreeid || candidate2 == outof_btreeid) {
		return;
	}
	//cand1 is in the middle of the tree cand2 is the leaf of the tree
	//swap first and find out the place to insert
	//cout << "move " << local_btree[candidate1].name << " " << local_btree[candidate2].name << endl;
	if (candidate1 == candidate2) return;
	if (block_given[candidate1].parent == &block_given[candidate2] || block_given[candidate2].parent == &block_given[candidate1]) return;
	swap(candidate1, candidate2);
	Block* candidate1_p = block_given[candidate1].parent;
	if (candidate1_p->l_child == &block_given[candidate1]) {
		candidate1_p->l_child = NULL;
	}
	else {
		candidate1_p->r_child = NULL;
	}
	int a;
	//find a position for insert
	do {
		a = rand() % block_given.size();
	} while (a == candidate1 || (block_given[a].l_child != NULL && block_given[a].r_child != NULL));
	a = candidate2;

	block_given[candidate1].parent = &block_given[a];
	//cout << block_given[candidate1].name <<" "<< block_given[a].name << endl;
	int y_boundary = block_given[a].y + block_given[candidate1].height;
	int x_boundary = block_given[a].x + block_given[a].width + block_given[candidate1].width;
	if (block_given[a].l_child == NULL && block_given[a].x + block_given[a].width + block_given[candidate1].width < Chip_width) {
		//cout << "ready to move to left" << endl;
		for (int i = 0; i < NumSoftModules; i++) {
			if (block_given[i].y < y_boundary && block_given[i].y + block_given[i].height > block_given[a].y) {
				if (block_given[i].x < x_boundary && block_given[i].x + block_given[i].width>block_given[a].x + block_given[a].width) {
					SA_success = false;
					return;
				}
			}
		}
		if (find_fixed(block_given[a].x + block_given[a].width, block_given[candidate1].width, block_given[a].y, block_given[candidate1].height) == -1) {
			//cout << "success move to left" << endl;
			block_given[a].l_child = &block_given[candidate1];
			return;
		}
	}
	if (block_given[a].r_child == NULL && block_given[a].y + block_given[a].height + block_given[candidate1].height < Chip_height) {

		for (int i = 0; i < NumSoftModules; i++) {
			if (block_given[i].y < y_boundary && block_given[i].y + block_given[i].height > block_given[a].y) {
				if (block_given[i].x < x_boundary && block_given[i].x + block_given[i].width>block_given[a].x + block_given[a].width) {
					SA_success = false;
					return;
				}
			}
		}
		//cout << "ready to move to right" << endl;
		if (find_fixed(block_given[a].x, block_given[candidate1].width, block_given[a].y + block_given[a].height, block_given[candidate1].height) == -1) {
			//cout << "success move to right" << endl;
			block_given[a].r_child = &block_given[candidate1];
		}
	}

	return;
}
void perturb() {
	int perturb_num = rand() % 12;
	//cout << perturb_num << endl;
	//int perturb_num = 1;
	//	find the soft module 
	int candidate1 = rand() % NumSoftModules;
	if (perturb_num < 2) {
		rotate(candidate1);
	}
	else if (perturb_num < 6) {
		int candidate2;
		//cout << "perturb 2 swap" << endl;
		do {
			candidate2 = rand() % NumSoftModules;
			//candidate2 = 23;
			//candidate1 = 27;
			if (local_btree[candidate1].parent == &local_btree[candidate2] || local_btree[candidate2].parent == &local_btree[candidate1]) {
				//cout <<"pc_swap " << local_btree[candidate1].name << " " << local_btree[candidate2].name << endl;
				pc_swap(candidate1, candidate2);
				break;
			}
		} while (candidate1 == candidate2);
		if (local_btree[candidate1].parent != &local_btree[candidate2] && local_btree[candidate2].parent != &local_btree[candidate1]) {
			//cout <<"normal_swap " << local_btree[candidate1].name << " " << local_btree[candidate2].name << endl;
			swap(candidate1, candidate2);
		}
	}
	else {
		int candidate2;
		candidate2 = candidate1;
		//2 is to find one leaf travel from 1
		while (local_btree[candidate2].l_child != NULL || local_btree[candidate2].r_child != NULL) {
			int randRL = rand() % 2;
			if (randRL) {
				if (local_btree[candidate2].l_child == NULL)
					candidate2 = block_mapid[local_btree[candidate2].r_child->name];
				else
					candidate2 = block_mapid[local_btree[candidate2].l_child->name];
			}
			else {
				if (local_btree[candidate2].r_child == NULL)
					candidate2 = block_mapid[local_btree[candidate2].l_child->name];
				else
					candidate2 = block_mapid[local_btree[candidate2].r_child->name];
			}
		}
		if (local_btree[candidate1].l_child == NULL && local_btree[candidate1].r_child == NULL) {
			move(candidate1, candidate2);
		}
	}
	return;
}
void clear_contour() {
	horzonital_contour.clear();
	horzonital_contour.assign(Chip_width * 2, 0);
	for (int i = NumSoftModules; i < block_given.size(); i++) {
		block_given[i].choose = false;
	}
	return;
}
void packing(Block* current, int dir) {//dir 0 for update lchild, dir 1 for update rchild
	if (current == local_root) {
		find_fixed(tmp_x, local_root->width, horzonital_contour[tmp_x], local_root->height);
		current->x = tmp_x;
		current->y = update_contour(tmp_x, current->width, current->height);
		if (find_fixed(current->x, current->width, current->y, current->height) != -1) {
			SA_success = false;
		}
	}
	else if (dir == 0) {
		//update left-child
		Block* parent = current->parent;
		int packing_x = parent->x + parent->width;
		find_fixed(packing_x, current->width, horzonital_contour[packing_x], current->height);
		int packing_y = update_contour(packing_x, current->width, current->height);
		if (find_fixed(packing_x, current->width, packing_y, current->height) != -1) {
			SA_success = false;
		}
		if (current->y + current->height < parent->y) {
			//SA_success = false;
		}
		current->x = packing_x;
		current->y = packing_y;
	}
	else {
		//dir == rightchild
		Block* parent = current->parent;
		tmp_x = parent->x;
		int packing_y = update_contour(tmp_x, current->width, current->height);
		if (find_fixed(tmp_x, current->width, packing_y, current->height) != -1) {
			SA_success = false;
		}
		current->x = tmp_x;
		current->y = packing_y;
	}
	//cout << current->name << " " << current->x << " " << current->y << endl;
	for (int i = 0; i < 10450; i += 1000) {
		//cout << horzonital_contour[i]<<" ";
	}
	//cout << endl;
	if (current->x + current->width > xboundary_max) {
		xboundary_max = current->x + current->width;
	}
	if (current->y + current->height > yboundary_max) {
		yboundary_max = current->y + current->height;
	}
	if (current->l_child != NULL) {
		packing(current->l_child, 0);
	}
	if (current->r_child != NULL) {
		packing(current->r_child, 1);
	}
	return;
}
void SA() {
	int get_worse = 0;
	//cout << "initial total weight " << total_weight(1) << endl;
	while (1) {
		//cout << "---------------SA----------------" << endl;
		//cout << "local_count" << local_count << endl;
		local_count++;
		store_local();
		//cout << "boundary " << xboundary_max << " " << yboundary_max << endl;
		perturb();
		//Preorder(local_root);
		xboundary_max = 0;
		yboundary_max = 0;
		SA_success = true;
		clear_contour();
		tmp_x = 0;
		packing(local_root, 0);
		local_weight = total_weight(2);
		sa_t2 = chrono::steady_clock::now();
		auto sa_time = chrono::duration_cast<chrono::microseconds>(sa_t2 - sa_t1).count();
		//cout << sa_time/ CLOCKS_PER_SEC << endl;
		if (get_worse >= 30000 || sa_time / CLOCKS_PER_SEC > 3000) {
			//Preorder(best_root);
			cout << get_worse << " " << sa_time / CLOCKS_PER_SEC<<endl;
			break;
		}
		if (xboundary_max > Chip_width || yboundary_max > Chip_height) { //out of boundary
			//cout << "out of boundary unavailiable" << endl;
			get_worse++;
			continue;
		}
		else if (!SA_success) { //overlap
			//cout << "overlap unavailiable" << endl;
			get_worse++;
		}
		else if (local_weight < best_weight) {
			restore_local();
			store_best();
			//cout << "success-----!!!!!best result!!!!!" << best_weight << endl;
			//Preorder(best_root);
			get_worse = 0;
		}
		else if (local_weight >= best_weight) {
			//cout << "success-----local result " << total_weight(2) << " " << total_weight(3) << endl;
			restore_local();
			get_worse++;
		}
		
	}
	return;
}

//int argc, char* argv[]
int main(int argc, char* argv[])
{
	//ifstream File_C("public3.txt"); //read file
	//ofstream output("public3.floorplan");
	ifstream File_C(argv[1]); //read net file
	ofstream output(argv[2]);
	string read_data;
	vector<string> v;
	/*
	* read file
	* */
	t1 = chrono::steady_clock::now();
	while (getline(File_C, read_data)) {
		if (read_data.length() == 0) {
			continue;
		}
		v = split_string(read_data);
		if (v[0] == "ChipSize") {
			Chip_width = stoi(v[1]);
			Chip_height = stoi(v[2]);
		}
		else if (v[0] == "NumSoftModules") {
			NumSoftModules = stoi(v[1]);
		}
		else if (v[0] == "SoftModule") {
			Block block;
			block.minarea = stoi(v[2]);
			block.name = v[1];
			block_given.push_back(block);
		}
		else if (v[0] == "NumFixedModules") {
			NumFixedModules = stoi(v[1]);
		}
		else if (v[0] == "FixedModule") {
			Block block;
			block.x = stoi(v[2]);
			block.y = stoi(v[3]);
			block.width = stoi(v[4]);
			block.height = stoi(v[5]);
			block.name = v[1];
			if (block.y + block.height == Chip_height && block.x == 0) {
				block.position = 8; //left up corner
			}
			else if (block.x + block.width == Chip_width && block.y + block.height == Chip_height) {
				block.position = 5; //right up corner
			}
			else if (block.x + block.width == Chip_width && block.y == 0) {
				block.position = 6; //right down corner
			}
			else if (block.y == 0 && block.x == 0) {
				block.position = 7; //left down corner
			}
			else if (block.y + block.height == Chip_height) {
				block.position = 1;
			}
			else if (block.y == 0) {
				block.position = 2;
			}
			else if (block.x == 0) {
				block.position = 3;
			}
			else if (block.x + block.width == Chip_width) {
				block.position = 4;
			}
			block_mapid[block.name] = block_given.size();
			block_given.push_back(block);
		}
		else if (v[0] == "NumNets") {
			NumNets = stoi(v[1]);
			sort_area();
			for (int i = 0; i < NumSoftModules; i++) {
				block_mapid[block_given[i].name] = i;
			}
		}
		else if (v[0] == "Net") {
			Net net;
			net.module_id.push_back(block_mapid[v[1]]);
			net.module_id.push_back(block_mapid[v[2]]);
			net.weight = stoi(v[3]);
			net_given.push_back(net);
		}
	}
	t2 = chrono::steady_clock::now();
	auto io_time = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	auto construct_data_structure_time = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	cout << "readfile finish" << endl;
	/*
	* decide soft module R
	* */
	t1 = chrono::steady_clock::now();
	for (int i = 0; i < NumSoftModules; i++) {
		int sqrt_of_area = sqrt(block_given[i].minarea);
		int width = sqrt_of_area;
		int height;
		bool find_apsect_ratio = false;
		height = block_given[i].minarea / width;
		if (width * height == block_given[i].minarea && (double)width / (double)height >= 0.5) {
			find_apsect_ratio = true;
		}
		//cout << block_given[i].minarea << " ";
		while (!find_apsect_ratio && (double)width / (double)height >= 0.5) {
			width--;
			height = block_given[i].minarea / width;
			if (width * height == block_given[i].minarea) {
				find_apsect_ratio = true;
			}
		}
		if (!find_apsect_ratio && (double)width / (double)height < 0.5) {
			width = sqrt_of_area + 1;
			height = width;
			block_given[i].minarea = width * height;
		}
		block_given[i].R = (double)width / (double)height;
		block_given[i].width = width;
		block_given[i].height = height;
		//cout << block_given[i].name << " " << block_given[i].width << " " << block_given[i].height << " " << block_given[i].minarea << endl;
	}
	cout << "apsect_ratio decide finish" << endl;
	/*
	* start initial floorplan
	* //
	* */
	initial_floorplan();
	cout <<"initial total weight " << total_weight(1) << endl;
	cout << "----------------------------------------" << endl;
	//Preorder(root);


	t2 = chrono::steady_clock::now();
	auto initial_floorplan_time = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	cout << "initial floorplan finish" << endl;
	store_local();
	store_best();
	/*
	* simulated anealing
	* */
	t1 = chrono::steady_clock::now();
	sa_t1 = chrono::steady_clock::now();
	SA();
	t2 = chrono::steady_clock::now();
	auto computing_part_time = chrono::duration_cast<chrono::microseconds>(t2 - t1).count();
	/*
	* write file
	* */
	//t1 = chrono::steady_clock::now();
	cout << "best total weight " << total_weight(3) << endl;
	output << "Wirelength " << total_weight(3) << endl;
	output << "NumSoftModules " << NumSoftModules << endl;
	for (int i = 0; i < NumSoftModules; i++) {
		output << best_btree[i].name << " " << best_btree[i].x << " " << best_btree[i].y << " " << best_btree[i].width << " " << best_btree[i].height << endl;;
	}
	for (int i = 0; i < NumSoftModules; i++) {
		//output << block_given[i].name << " " << block_given[i].x << " " << block_given[i].y << " " << block_given[i].width << " " << block_given[i].height << endl;;
	}
	for (int i = 0; i < NumSoftModules; i++) {
		//output << local_btree[i].name << " " << local_btree[i].x << " " << local_btree[i].y << " " << local_btree[i].width << " " << local_btree[i].height << endl;;
	}
	Preorder(best_root);
	//t2 = chrono::steady_clock::now();
	//io_time += chrono::duration_cast<chrono::microseconds>(t2 - t1).count();

	/*
	* execution time
	* */
	//cout << "I/O time : " << io_time << endl;
	//cout << "Constructing data structure time : " << construct_data_structure_time << endl;
	//cout << "Initial floorplanning time : " << initial_floorplan_time << endl;
	//cout << "computing part time : " << computing_part_time << endl;
	return 0;
}