
#include<easyx.h>//引用图形库头文件
#include <conio.h>
#include <time.h>
#include <windows.h>
#include <cmath>
#pragma  comment(lib,"Msimg32.lib")//贴图
#include <cstdio>
#include <fstream>
using namespace std;

////////// 需要的结构体 //////////
struct Food {
	int x;
	int y;
	int r;
	COLORREF c;
};
struct Node {
	int x;
	int y;
	Node* nextNode;
	Node* lastNode;
	Node(int x, int y) {
		this->x = x;
		this->y = y;
		nextNode = nullptr;
		lastNode = nullptr;
	}
};


//////需要的函数//////
void putTimage(int x, int y, IMAGE* srcimg, IMAGE* dstimg = NULL, UINT transparentcolor = 0)
{
	HDC dstDC = GetImageHDC(dstimg);
	HDC srcDC = GetImageHDC(srcimg);
	int w = srcimg->getwidth();
	int h = srcimg->getheight();
	// 使用 Windows GDI 函数实现透明位图
	TransparentBlt(dstDC, x, y, w, h, srcDC, 0, 0, w, h, transparentcolor);
}


//////全局变量//////
const int ScreenWidth = 640;//窗口宽度
const int ScreenHeight = 480;
const int SideX = 10;//边缘宽度
const int SideY = 10;
IMAGE* imgMap = new IMAGE(ScreenWidth * 4, ScreenHeight * 4);//世界地图
const int GameWidth = imgMap->getwidth() - 2 * SideX;//游戏宽度
const int GameHeight = imgMap->getheight() - 2 * SideY;
int nFood = 300;
Food* food;//食物动态数组
const COLORREF playerColor0 = 0x1196EE;
const COLORREF playerColor1 = RGB(200, 0, 0);
const COLORREF playerColor2 = RGB(255, 255, 0);
const int SizeNode = 20;//蛇的大小
const int moveTimes = 5;//times取值5 4 3 2 1
const int stepLen = 4;
const int PI = 3.1415926;
int mapX = 0;
int mapY = 0;
long ret = 0;
const int AiSum = 15;
MOUSEMSG msg;


////////// 双向链表实现蛇类 //////////
class SnakeBase {
public:
	SnakeBase() {
		eaten = 40;
		nNode = eaten / 5;
		imgHead = imgNode1 = imgNode2 = nullptr;
		headNode = tailNode = nullptr;
	}
	//设计蛇的图案
	void SetImage(COLORREF headColor, COLORREF nodeColor1, COLORREF nodeColor2) {
		imgHead = new IMAGE(SizeNode, SizeNode);
		imgNode1 = new IMAGE(SizeNode, SizeNode);
		imgNode2 = new IMAGE(SizeNode, SizeNode);
		//蛇头的形状
		SetWorkingImage(imgHead);
		setfillcolor(nodeColor2);
		solidcircle(SizeNode / 2, SizeNode / 2, SizeNode / 2);
		setfillcolor(WHITE);
		solidcircle(SizeNode / 2, SizeNode / 2, 6);
		setfillcolor(0x002933);
		solidcircle(SizeNode / 2, SizeNode / 2, 3);

		SetWorkingImage(imgNode1);
		setfillcolor(nodeColor1);
		solidcircle(SizeNode / 2, SizeNode / 2, SizeNode / 2);

		SetWorkingImage(imgNode2);
		setfillcolor(nodeColor2);
		solidcircle(SizeNode / 2, SizeNode / 2, SizeNode / 2);
		SetWorkingImage();
	}
	//创建节点
	void CreateSnake(int headX, int headY) {
		headNode = new Node(headX, headY);
		Node* temp = headNode;
		for (int i = 0; i < nNode - 1; i++)
		{
			Node* newNode = new Node(headX, headY);
			newNode->lastNode = temp;
			temp->nextNode = newNode;
			temp = newNode;
		}
		temp->nextNode = nullptr;
		tailNode = temp;
		temp = nullptr;
	}
	//绘制蛇身
	void DrawSnake() {
		Node* temp = tailNode;
		int n = nNode;
		while (temp != headNode) {
			putTimage(temp->x - SizeNode / 2 + mapX, temp->y - SizeNode / 2 + mapY, n % 2 == 0 ? imgNode1 : imgNode2);
			temp = temp->lastNode;
			n++;
		}
		putTimage(temp->x - SizeNode / 2 + mapX, temp->y - SizeNode / 2 + mapY,imgHead);
		temp = nullptr;
	}
	//刷新数据函数
	void FlushData(int times, int dx, int dy) {
		//存储之前的坐标
		if (times == moveTimes) {
			nodeMsg = new POINT[nNode];
			Node* temp = headNode;
			int i = 0;
			while (temp != nullptr)
			{
				nodeMsg[i].x = temp->x;
				nodeMsg[i].y = temp->y;
				temp = temp->nextNode;
				i++;
			}
		}
		//头节点移动
		headNode->x += dx;
		headNode->y += dy;
		//其他节点移动
		Node* temp = tailNode;
		int i = nNode - 2;
		while (temp != headNode)
		{
			//最后一次直接变为之前的坐标
			if (times == 1)
			{
				temp->x = nodeMsg[i].x;
				temp->y = nodeMsg[i].y;
			}
			//其他次数中慢慢前移
			else
			{
				temp->x += int(stepLen * cos(atan2(nodeMsg[i].y - temp->y, nodeMsg[i].x - temp->x)));
				temp->y += int(stepLen * sin(atan2(nodeMsg[i].y - temp->y, nodeMsg[i].x - temp->x)));
			}
			temp = temp->lastNode;
			i--;
		}
		temp = nullptr;
		//最后一次清空之前的数据
		if (times == 1)
		{
			delete[] nodeMsg;
			nodeMsg = nullptr;
		}
	}
	//判断是否吃到食物
	bool GetFood(int i, int x, int y) {
		int r = food[i].r;
		if (pow(x - headNode->x, 2) + pow(y - headNode->y, 2) < pow((SizeNode/2 + r), 2)) {
			food[i].x = rand() % (GameWidth - 16) + SideX + 8;
			food[i].y = rand() % (GameHeight - 16) + SideY + 8;
			food[i].r = rand() % 2 + 3;
			food[i].c = RGB(rand() % 256, rand() % 256, rand() % 256);
			return true;
		}
		return false;
	}
	//根据吃的食物添加节点
	void SetNode(int eat) {
		int temp = nNode;
		eaten += eat;
		if (eaten/5 - temp > 0) {
			nNode++;
			Node* tempNode = new Node(tailNode->x, tailNode->y);
			tempNode->lastNode = tailNode;
			tailNode->nextNode = tempNode;
			tailNode = tempNode;
			tempNode = nullptr;
		}
	}
	int GetEat()const{
		return eaten;
	}
	int GetNode()const {
		return nNode;
	}
public:
	Node* headNode, * tailNode;
protected:
	int eaten;
	int nNode;
	IMAGE* imgHead, * imgNode1, * imgNode2;
	POINT* nodeMsg;
};

////////// Player 类 //////////
class Player :public SnakeBase {
public:
	Player() :SnakeBase() {
		SetImage(playerColor0, playerColor1, playerColor2);
		int headX = rand() % (GameWidth - 2 * SizeNode) + SideX + SizeNode;
		int headY = rand() % (GameHeight - 2 * SizeNode) + SideY + SizeNode;
		CreateSnake(headX, headY);
		times = moveTimes;
		dx = 0;
		dy = 0;
		exp = 0;
	}
	//移动
	void Move() {
		mapX -= dx;
		mapY -= dy;
		FlushData(times, dx, dy);
		times--;
		for (int i = 0; i < nFood; i++) {
			if (GetFood(i, food[i].x, food[i].y)) {
				exp += food[i].r / 2;
			}
		}
		if (times == 0) {
			times = moveTimes;
			while (MouseHit()) {
				msg = GetMouseMsg();
			}
			//计算距离原点的角度
			rad = atan2(msg.y - ScreenHeight / 2, msg.x - ScreenWidth / 2);
			dx = int(stepLen * cos(rad));
			dy = int(stepLen * sin(rad));
			SetNode(exp);
			exp = 0;
		}
	}
	//是否碰到AI
	bool IsInAi(int AIX, int AIY) {
		if (pow(headNode->x - AIX, 2) + pow(headNode->y - AIY, 2) < pow(SizeNode, 2)) {
			return true;
		}
		return false;
	}
protected:
	int times;
	int dx, dy;
	double rad;
	int exp;
};

////////// AI 类 //////////
class AI :public SnakeBase {
public:
	AI(Player *p) :SnakeBase() {
		this->p = p;
		COLORREF c0 = HSVtoRGB(float(rand() % 360), 1.0f, 0.9f);
		COLORREF c1 = HSVtoRGB(float(rand() % 360), 1.0f, 0.8f);
		COLORREF c2 = HSVtoRGB(float(rand() % 360), 1.0f, 1.0f);
		SetImage(c0, c1, c2);
		int headX = 0;
		int headY = 0;
		do
		{
			headX = rand() % (GameWidth - 2 * SizeNode) + SideX + SizeNode;
			headY = rand() % (GameHeight - 2 * SizeNode) + SideY + SizeNode;
		} while (IsInPlayer(headX, headY));
		CreateSnake(headX, headY);
		times = moveTimes;
		rad = (rand() % 360);
		dx = int(stepLen * cos(rad));
		dy = int(stepLen * sin(rad));
		curLine = (3 + rand() % 12) * moveTimes;
		exp = 0;
		isFast = false;
	}
	//自动移动
	void Move() {
		if (curLine == 0) {
			curLine = (3 + rand() % 12) * moveTimes;
			rad = (rand() % 360);
			dx = int(stepLen * cos(rad));
			dy= int(stepLen * sin(rad));
			isFast = rand() % 1000 < 150 ? true : false;//有百分之15的概率加速
		}
		FlushData(times, dx, dy);
		times--;
		curLine--;
		//判断是否吃到食物
		for (int i = 0; i < nFood; i++) {
			if (GetFood(i, food[i].x, food[i].y)) {
				exp += food[i].r / 2;
			}
		}
		//移动过去后
		if (times == 0) {
			times = moveTimes;
			SetNode(exp);
			exp = 0;
			//如果距离墙壁只有一个球的距离
			if (headNode->x < SizeNode + SideX || GameWidth + SideX - 2 * SizeNode < headNode->x ) {
				dx *= -1;
			}
			if (headNode->y - SideY < SizeNode || GameHeight + SideY - headNode->y < 2 * SizeNode) {
				dy *= -1;
			}
			rad = atan2(dy, dx);
			dx = int(stepLen * cos(rad));
			dy = int(stepLen * sin(rad));
			int deadX = headNode->x + SizeNode * cos(rad);
			int deadY = headNode->y + SizeNode * sin(rad);
			//如果距离玩家约一个球的距离 
			if (IsInPlayer(deadX, deadY)) {
				rad += 180;
				dx = int(stepLen * cos(rad));
				dy = int(stepLen * sin(rad));
			}
		}
		//如果死亡变成食物
		if (IsDead()) {
			Node* temp = headNode;
			int n = rand() % (nFood - nNode);
			for (int i = 0; i < nNode; i++) {
				food[n].x = temp->x;
				food[n].y = temp->y;
				food[n].r = SizeNode / 2;
				food[n].c = RGB(rand() % 256, rand() % 256, rand() % 256);
				temp = temp->nextNode;
				n++;
			}
			temp = nullptr;
		}
	}
	// 位置是否接触 player
	bool IsInPlayer(int headX, int headY)
	{
		Node* temp = p->headNode;
		while (temp != nullptr)
		{
			if (pow(headX - temp->x, 2) + pow(headY - temp->y, 2) < pow(SizeNode, 2))
				return true;
			temp = temp->nextNode;
		}
		return false;
	}
	//判定死亡
	bool IsDead() {
		if (headNode->x <= SideX || headNode->x >= GameWidth + SideX - SizeNode || headNode->y <= SideY || headNode->y >= GameHeight + SideY - SizeNode) {
			return true;
		}
		if (IsInPlayer(headNode->x, headNode->y)) {
			return true;
		}
		return false;
	}
protected:
	int times;
	int dx, dy;
	int curLine;//拐弯的长度
	int exp;
	double rad;
	Player* p;
public:
	clock_t ct;
	bool isFast;
};
struct Ai
{
	AI* ais;
	struct Ai* next;
};

//////游戏类//////
class Game {
public:
	Game(int record) {
		this->record = record;
		BeginBatchDraw();
		//设置背景
		SetWorkingImage(imgMap);
		setbkcolor(BLACK);
		cleardevice();
		SetWorkingImage();

		putimage(0, 0, imgMap);
		img = new IMAGE(GameWidth, GameHeight);
		loadimage(img, L"bg.jpg", GameWidth, GameHeight);
		putimage(SideX, SideY, img);
		
		//初始化食物
		food = new Food[nFood];
		for (int i = 0; i < nFood; i++) {
			food[i].x = rand() % (GameWidth - 8) + SideX;
			food[i].y = rand() % (GameHeight - 8) + SideY;
			food[i].r = rand() % 2 + 3;
			food[i].c = RGB(rand() % 256, rand() % 256, rand() % 256);
		}
		//初始化蛇
		int i = 1;
		p = new Player();
		ai = new Ai();
		ai->ais = new AI(p);
		Ai* temp = ai;
		for (int i = 1; i < AiSum; i++)
		{
			temp->next = new Ai();
			temp = temp->next;
			temp->ais = new AI(p);
		}
		temp->next = ai;
		temp = nullptr;
		mapX = -1 * (p->headNode->x - ScreenWidth / 2) + SideX;
		mapY = -1 * (p->headNode->y - ScreenHeight / 2) + SideY;
		//画蛇
		Draw();
		//蛇的移动数据
		flush = 30;
		FlushBatchDraw();
	}
	//画食物
	void DrawFood() {
		for (int i = 0; i < nFood; i++) {
			switch (food[i].r)
			{
			case(4):
				setfillcolor(food[i].c);
				solidcircle(food[i].x + mapX, food[i].y + mapY, food[i].r);
				break;
			case(3):
				setfillcolor(food[i].c);
				solidcircle(food[i].x + mapX, food[i].y + mapY, food[i].r);
				break;
			default:
				setfillcolor(food[i].c);
				solidcircle(food[i].x + mapX, food[i].y + mapY, food[i].r);
				setfillcolor(WHITE);
				solidcircle(food[i].x + mapX, food[i].y + mapY, food[i].r / 2);
				break;
			}
		}
	}
	//画蛇
	void Draw() {
		putimage(0, 0, imgMap);
		putimage(mapX, mapY, img);
		//画食物
		DrawFood();
		p->DrawSnake();
		Ai* temp = ai->next;
		while (temp != ai)
		{
			temp->ais->DrawSnake();
			temp = temp->next;
		}
		temp->ais->DrawSnake();
		temp = nullptr;
		//打印成绩
		Print();
		FlushBatchDraw();
	}
	//开始游戏
	void Running() {
		bool isFast = false;
		Ai* temp = ai->next;
		while (temp != ai) {
			temp->ais->ct = clock();
			temp = temp->next;
		}
		temp->ais->ct = clock();
		time_t ct = clock();
		while (!IsOver()) {
			if (GetAsyncKeyState('P') & 0x8000)
				ret = _getwch();
			//当某条ai蛇过了flush移动一下
			if (clock() - temp->ais->ct > flush) {
				temp->ais->ct = clock();
				temp->ais->Move();
				if (temp->ais->IsDead()) {
					delete  temp->ais;
					temp->ais = new AI(p);
					temp->ais->ct = clock();
				}
				else if (temp->ais->isFast) {
					temp->ais->Move();
				}
				temp = temp->next;
			}
			//当玩家蛇过了flush移动一下,
			if (clock() - ct > flush) {
				ct = clock();
				isFast = (msg.mkLButton || msg.mkRButton) ? true : false;
				p->Move();
				if (isFast) {
					p->Move();
				}
				Draw();//在画图之前所有的AI蛇都移动完毕
			}
		}
	}
	//判定游戏是否结束
	bool IsOver() {
		int headX = p->headNode->x;
		int headY = p->headNode->y;
		//碰墙死亡
		if (headX <= SideX || headX >= GameWidth + SideX - SizeNode || headY <= SideY || headY >= GameHeight + SideY - SizeNode) {
			return true;
		}
		//撞到ai死亡
		Ai* temp = ai;
		for (int i = 0; i < AiSum; i++) {
			Node* node = temp->ais->headNode;
			while (node != nullptr) {
				if (p->IsInAi(node->x, node->y)) {
					return true;
				}
				node = node->nextNode;
			}
			temp = temp->next;
		}
		return false;
	}
	//打印当前节数
	void Print() {
		Ai* temp = ai;
		int length = p->GetNode();
		int rank = 1;
		for (int i = 0; i < AiSum; i++) {
			if (temp->ais->GetNode() >= length) {
				rank++;
			}
			temp = temp->next;
		}
		setbkmode(TRANSPARENT);
		settextcolor(BLACK);
		setfillcolor(WHITE);
		solidroundrect(530,20,630,200,10,10);
		settextstyle(20, 0, _T("微软雅黑"));
		outtextxy(550, 25, _T("当前信息"));
		TCHAR str[32] = { 0 };
		_stprintf_s(str, _T("食物：%d"), p->GetEat());
		outtextxy(550, 50, str);
		_stprintf_s(str, _T("长度：%d"), p->GetNode());
		outtextxy(550, 75, str);
		temp = nullptr;
		_stprintf_s(str, _T("排名：%d"), rank);
		outtextxy(550, 100, str);
		settextcolor(RED);
		_stprintf_s(str, _T("最长长度：%d"), record);
		outtextxy(535, 125, str);
	}
public:
	IMAGE* img;
	Ai* ai;
	int flush;
	Player* p;
	int record;
};
int main() {
	initgraph(ScreenWidth, ScreenHeight);
	srand(time(NULL));
	int record = 0;
	fstream f;
	f.open("gamerecord.txt", ios::in);
	f >> record;
	f.close();
	Game game(record);
	settextcolor(WHITE);
	settextstyle(50, 0, _T("微软雅黑"));
	outtextxy((ScreenWidth - textwidth(_T("按任意键开始游戏"))) / 2, ScreenHeight / 3, _T("按任意键开始游戏"));
	settextstyle(20, 0, _T("微软雅黑"));
	outtextxy((ScreenWidth - textwidth(_T("鼠标控制方向"))) / 2, ScreenHeight / 4 * 3 + 50, _T("鼠标控制方向"));
	outtextxy((ScreenWidth - textwidth(_T("P暂停"))) / 2, ScreenHeight / 4 * 3 + 70, _T("P暂停"));
	FlushBatchDraw();
	ret = _getwch();
	game.Running();
	f.open("gamerecord.txt", ios::out);
	if (game.p->GetNode() > record) {
		f << game.p->GetNode();
	}
	else {
		f << record;
	}
	f.close();
	settextcolor(WHITE);
	settextstyle(50, 0, _T("微软雅黑"));
	outtextxy((ScreenWidth - textwidth(_T("游戏结束"))) / 2, ScreenHeight / 3, _T("游戏结束"));
	settextstyle(20, 0, _T("微软雅黑"));
	outtextxy((ScreenWidth - textwidth(_T("按任意键退出"))) / 2, ScreenHeight / 4 * 3 + 50, _T("按任意键退出"));
	FlushBatchDraw();
	ret = _getwch();
	return 0;
}