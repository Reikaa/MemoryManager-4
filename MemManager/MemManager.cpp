#include "MemManager.h"

/////////////////////////////////////////////
// Globalne pole running
PROTEZA::PCB *running;

void PROTEZA::showProcessList() {

	for (auto x : process_list) {
		std::cout << "PROCESS: " << x->name << " PID " << x->PID << " LR: " << x->LR << "\n Page Table: ";
		MemoryManager::showPageTable(x->page_table);
	}
	std::cout << std::endl;
};

void PROTEZA::showRunning() {
	if (running == nullptr) {
		std::cout << "Nie ma procesu running" << std::endl;
		return;
	}
	std::cout << "RUNNING PROCESS: " << running->name << " PID " << running->PID << " LR: " << running->LR;
	std::cout << std::endl;
};

void PROTEZA::selectRunning(int PID) {
	std::cout << "Zmiana procesu running" << std::endl;

	for (int i = 0; i < process_list.size(); i++) {
		if (PID == process_list.at(i)->PID) {
			running = process_list.at(i);
			break;
		}
	}
	//std::cout << "Nie znaleziono procesu!" << std::endl;
};

void PROTEZA::GO(MemoryManager &mm) {
	if (running == nullptr) {
		std::cout << "Nie ma procesu running" << std::endl;
		return;
	}

	std::cout << "Wykonany rozkaz: ";
	std::string rozkaz = mm.Get(running->LR);
	std::cout << rozkaz << " ";
	running->LR += rozkaz.size()+1;

	while (true) {
		rozkaz = mm.Get(running->LR);
		if (rozkaz.size() == 0 || isdigit(rozkaz[0])) {
			std::cout << rozkaz << " ";
			running->LR += rozkaz.size() + 1;
			break;
		}
		else 
			std::cout << rozkaz << " ";

		running->LR += rozkaz.size() + 1;
	}
	std::cout <<std::endl;
};

void PROTEZA::removeProcess(int PID, MemoryManager &mm) {

	for (int i = 0; i < process_list.size(); i++) {
		if (PID == process_list[i]->PID) {
			if (running == process_list[i])
				running = nullptr;
			delete process_list[i];
			process_list.erase(process_list.begin() + i);
			mm.Remove(PID);
		}
	}
}

bool PROTEZA::checkPID( int n) {
	for (auto x : process_list)
		if (x->PID == n) {
			std::cout << "Proces z takim PID istnieje " << std::endl;
			return false;
		}

	return true;
};

/////////////////////////////////////////////

void MemoryManager::start() {
	//wypelniam ram spacjami
	for (int i = 0; i < 128; i++) {
		RAM[i] = ' ';
	}
	std::string data = "JP 0";
	std::vector<Page> Pvec{ Page(data) };
	PageFile.insert(std::make_pair(0, Pvec));
}

void MemoryManager::showFrameList() {
	std::cout << "\t FREE \t PAGE \t PID" << std::endl;
	for (int i = 0; i < 8; i++) {
		std::cout << "FRAME " << i << ":  " << Frames[i].free << " \t " << Frames[i].pageN << " \t " << Frames[i].PID << std::endl;
	}
}

void MemoryManager::showPMemory(int start, int amount) {
	if (start + amount > 128)
		std::cout << "Nie mozna przeczytac pamieci podano zly zakres" << std::endl;
	else {
		std::cout << "Pamiec fizyczna od adresu " << start << " do adresu " << start + amount << std::endl;
		for (int i = start; i < amount + start; i++) {
			//nowa linia po 16 znakach
			if (i % 16 == 0 && i != 0)
				std::cout << std::endl;
			//zamieniamy spacje na _ dla ulatwienia odczytu
			if (RAM[i] != ' ')
				std::cout << RAM[i];
			else
				std::cout << '_';

		}
		std::cout << std::endl;
	}
}

void MemoryManager::showPMemory() {
	for (int i = 0; i < 128; i++) {
		if (i % 16 == 0)
			std::cout << std::endl << "FRAME " << i / 16 << " ";
		if (RAM[i] != ' ')
			std::cout << RAM[i];
		else
			std::cout << '_';

	}
	std::cout << std::endl;
}

void MemoryManager::showPageTable(std::vector<PageTableData> *page_table) {
	//print running->PID
	for (int i = 0; i < page_table->size(); i++)
		std::cout << page_table->at(i).frame << " " << page_table->at(i).bit << std::endl;
}

void MemoryManager::ShowPageFile() {
	for (auto x : PageFile) {
		std::cout << "PROCESS ID: " << x.first << std::endl;
		for (int i = 0; i < x.second.size(); i++) {
			x.second.at(i).print();
		}
	}
}

void MemoryManager::ShowLRUList() {
	for (auto it : LRU) {
		std::cout << it << " ";
	}
}

void MemoryManager::FrameOrder(int Frame) {
	if (Frame > 7)
		return;

	for (auto it = LRU.begin(); it != LRU.end(); it++) {
		if (*it == Frame) {
			LRU.erase(it);
			break;
		}
	}
	LRU.push_front(Frame);
}

int MemoryManager::seekForFreeFrame() {
	int Frame = -1;
	for (int i = 0; i < Frames.size(); i++) {
		if (Frames[i].free == 1) {
			Frame = i;
			break;
		}
	}
	return Frame;
}

int MemoryManager::LoadtoMemory(Page page, int pageN, int PID, std::vector<PageTableData> *page_table) {
	int n = 0;
	int Frame = -1;
	Frame = seekForFreeFrame();
	if (Frame == -1)//Nie ma wolnych 
		Frame = SwapPages(page_table, pageN, PID);
	int end = Frame * 16 + 16;

	for (int i = Frame * 16; i < end; i++) {
		RAM[i] = page.data[n];
		n++;
	}

	//Zmieniamy wartosci w tablicy stronic
	page_table->at(pageN).bit = 1;
	page_table->at(pageN).frame = Frame;

	//Uzylismy ramki wiec wedruje na poczatek listy LRU oraz oznaczamy ja jako zajêt¹
	FrameOrder(Frame);
	Frames[Frame].free = 0;
	Frames[Frame].pageN = pageN;
	Frames[Frame].page_table = page_table;
	Frames[Frame].PID = PID;

	return Frame;
}

int MemoryManager::LoadProgram(std::string path, int mem, int PID) {
	double pages = ceil((double)mem / 16);
	std::fstream file; // zmienna pliku
	std::string str; // zmienna pomocnicza
	std::string program; // caly program w jedym lancuchu
	std::vector<Page> pagevec; // vector zawierajacy stronice do dodania
	file.open(path, std::ios::in);
	if (!file.is_open()) {
		std::cout << "Nie moge otworzyc pliku" << std::endl;
		return -1;
	}

	//std::cout << "Czytam program o nazwie "<<path<<std::endl;
	while (std::getline(file, str)) {
		//dodaje spacje zamiast konca lini
		if (!file.eof())
			str += " ";
		program += str;
	}
	str.clear();

	//Dziele program na stronice
	for (int i = 0; i < program.size(); i++) {
		str += program[i];
		//toworzymy stronice
		if (str.size() == 16) {
			pagevec.push_back(Page(str));
			str.clear();
		}
	}

	//ostatnia stronica
	if (str.size() != 0)
		pagevec.push_back(Page(str));
	str.clear();

	//Jezeli przydzielono za malo pamieci wywalam blad
	if (pages*16 < 16 * pagevec.size()) {
		std::cout << "Przydzielono za malo pamieci programowi" << std::endl;
		return -1;
	}

	//Sprawdzam czy program nie potrzebuje dodatkowej pustej stronicy (wolne miejsce w programie)
	for (int i = pagevec.size(); i < pages; i++)
		pagevec.push_back(str);

	//Dodanie stronic do pliku wymiany
	PageFile.insert(std::make_pair(PID, pagevec));

	return 1;
}

int MemoryManager::SwapPages(std::vector<PageTableData> *page_table, int pageN, int PID) {
	//*it numer ramki ktora jest ofiara
	auto it = LRU.end(); it--;
	int Frame = *it;
	// Przepisuje zawartosc z ramki ofiary do Pliku wymiany
	for (int i = Frame * 16; i < Frame * 16 + 16; i++) {
		PageFile[Frames[Frame].PID][Frames[Frame].pageN].data[i - (Frame * 16)] = RAM[i];
	}

	//Zmieniam wartosci w tablicy stronic ofiary
	Frames[Frame].page_table->at(Frames[Frame].pageN).bit = 0;
	Frames[Frame].page_table->at(Frames[Frame].pageN).frame = -1;

	return Frame;
}

std::vector<MemoryManager::PageTableData>* MemoryManager::createPageTable(int mem, int PID) {
	double pages = ceil((double)mem / 16);
	int Frame = -1;
	std::vector<PageTableData> *page_table = new std::vector<PageTableData>();

	//Tworzenie tablicy
	for (int i = 0; i < pages; i++)
		page_table->push_back(PageTableData(-1, 0));

	//Zaladowanie pierwszej stronicy do pamieci fizycznej
	LoadtoMemory(PageFile.at(PID).at(0), 0, PID, page_table);

	return page_table;
}

std::string MemoryManager::Get(int LR) {
	//string do wyslania
	std::string order;
	bool koniec = false;
	int Frame = -1;
	int stronica = LR / 16;

	// przekroczenie zakresu dla tego procesu
	if (running->page_table->size() <= stronica) {
		std::cout << "Przekroczenie zakresu";
		koniec = true;
		order = "ERROR";
	}
	while (!koniec) {
		stronica = LR / 16; //stronica ktora musi znajdowac sie w pamieci
		// koniec programu
		if (running->page_table->size() <= stronica) {
			koniec = true;
			break;
		}

		//brak stronicy w pamieci operacyjnej
		if (running->page_table->at(stronica).bit == 0)
			LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);
		
		//stronica w pamieci operacyjnej
		if (running->page_table->at(stronica).bit == 1) {
			Frame = running->page_table->at(stronica).frame;		//Ramka w ktorej pracuje
			FrameOrder(Frame);									//uzywam ramki wiec zmieniam jej pozycje na liscie LRU
			if (RAM[Frame * 16 + LR - (16 * stronica)] == ' ')	//Czytam do napotkania spacji
				koniec = true;
			else
				order += RAM[Frame * 16 + LR - (16 * stronica)];
		}
		LR++;
	}
	return order;
}

void MemoryManager::Remove(int PID) {
	for (int i = 0; i < Frames.size(); i++) {
		if (Frames.at(i).PID == PID) { // Zerowanie pamieci RAM
			for (int j = i * 16; j < i * 16 + 16; j++) {
				RAM[j] = ' ';
			}

			Frames.at(i).free = 1; // Komorka znowu wolna
			Frames.at(i).pageN = -1;
			Frames.at(i).PID = -1;
			PageFile.erase(PID);
			//delete Frames.at(i).page_table; // usuwanie tablicy stronic dla procesu
		}
	}
}

int MemoryManager::Write(int adress, char ch) {
	int stronica = adress / 16;
	std::string str;

	if (adress > running->page_table->size() * 16 - 1) {
		std::cout << "Przekroczona zakres dla tego procesu" << std::endl;
		return -1;
	}

	//sprawdzam czy stronica do ktorej chce zapisac jest w pamieci
	if (running->page_table->at(stronica).bit == 0)
		LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);

	str = PageFile[running->PID].at(stronica).data;

	//sprawdzam czy wybrane miejsce jest wolne
	if (str[(stronica * 16 - adress)* -1] != ' ') {
		std::cout << "Nie wpisano znaku do pamieci podano zajety adres" << std::endl;
		return -1;
	}

	//sprwadzam czy nastepne i poprzednie miejsca sa wolne
	if (adress < running->page_table->size() * 16 -1 && (adress + 1) / 16 > stronica) {
	//	std::cout << "MUSZE WCZYTAC NASTEPNA!";
		//if (running->page_table->at(stronica+1).bit == 0)
			//LoadtoMemory(PageFile[running->PID][stronica+1], stronica+1, running->PID, running->page_table);
		std::string pom = PageFile[running->PID].at(stronica+1).data;
		if (pom[0] != ' ') {
			std::cout << "Nie wpisano znaku do pamieci podano zajety adres" << std::endl;
			return -1;
		}
	}
	else if(adress != running->page_table->size() * 16 - 1) {
		if (str[(stronica * 16 - adress-1)* -1] != ' ')
			return -1;
	}

	if (adress > 0 && (adress - 1) / 16 < stronica) {
	//	std::cout << "MUSZE WCZYTAC POPRZEDNIA!";
		//if (running->page_table->at(stronica - 1).bit == 0)
			//LoadtoMemory(PageFile[running->PID][stronica-1], stronica-1, running->PID, running->page_table);
		std::string pom = PageFile[running->PID].at(stronica - 1).data;
		if (pom[15] != ' ') {
			std::cout << "Nie wpisano znaku do pamieci podano zajety adres"<<std::endl;
			return -1;
		}
	}
	else if(adress !=0) {
		if (str[(stronica * 16 - adress+1)* -1] != ' ')
			return -1;
	}

	//wpisanie do pamieci
	RAM[running->page_table->at(stronica).frame * 16 + adress - (16 * stronica)] = ch;
}

//int MemoryManager::wr(int adress,std::string data) {
//	int prev = adress - 1;
//	int next = data.size()+adress;
//	int stronica = adress / 16;
//	std::string str;
//
//	if (adress > running->page_table->size() * 16 - 1 || adress < 0) {
//		std::cout << "Przekroczona zakres dla tego procesu" << std::endl;
//		return -1;
//	}
//
//	//sprawdzam czy stronica do ktorej chce zapisac jest w pamieci
//	if (running->page_table->at(stronica).bit == 0)
//		LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);
//
//	for (int i = 0; i < 16; i++)
//		str += RAM[running->page_table->at(stronica).frame + i];
//	std::cout << str << std::endl;
//
//	//sprawdzam czy wybrane miejsce jest wolne
//	if (str[(stronica * 16 - adress)* -1] != ' ') {
//		std::cout << "Nie wpisano znaku do pamieci podano zajety adres" << std::endl;
//		return -1;
//	}
//
//	//sprawdzam poprzedni
//	if (adress != 0) {
//		//sprawdzam w poprzedniej stronicy
//		if (prev / 16 != stronica) {
//			std::string pom = PageFile[running->PID].at(stronica - 1).data;
//			if (pom[15] != ' ') {
//				std::cout << "Nie wpisano znaku do pamieci podano zajety adres" << std::endl;
//				return -1;
//			}
//		}
//		//sprwadzam w tej samej stornicy
//		else {
//			if (str[(stronica * 16 - prev)* -1] != ' ')
//				return -1;
//		}
//	}
//
//	//sprawdzam nastepny
//	/*if (adress != running->page_table->size() * 16 - 1) {
//		if (next / 16 != stronica) {
//			std::string pom = PageFile[running->PID].at(stronica + 1).data;
//			if (pom[0] != ' ') {
//				std::cout << "Nie wpisano znaku do pamieci podano zajety adres" << std::endl;
//				return -1;
//			}
//		}
//		else {
//			if (str[(stronica * 16 - next)* -1] != ' ')
//				return -1; \
//		}
//	}
//	*/
//	int ile_Stronic = (adress - (stronica * 16) + data.size()) / 16;
//
//	for (int i = 0; i < data.size(); i++) {
//		stronica = (adress + 1) / 16;
//
//		if (running->page_table->at(stronica).bit == 0)
//			LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);
//
//		RAM[running->page_table->at(stronica).frame * 16 + adress + i - (16 * stronica)] = data[i];
//		FrameOrder(running->page_table->at(stronica).frame);
//	}
//	
//	return 1;
//}

int MemoryManager::wr(int adress, std::string data) {
	int stronica = adress / 16;
	std::string str;

	if (adress+data.size()-1 > running->page_table->size() * 16 - 1 || adress < 0) {
		std::cout << "Przekroczona zakres dla tego procesu" << std::endl;
		return -1;
	}

	for (int i = 0; i < data.size(); i++) {
		stronica = (adress + i) / 16;

		if (running->page_table->at(stronica).bit == 0)
			LoadtoMemory(PageFile[running->PID][stronica], stronica, running->PID, running->page_table);

		RAM[running->page_table->at(stronica).frame * 16 + adress + i - (16 * stronica)] = data[i];
		FrameOrder(running->page_table->at(stronica).frame);
	}

	return 1;
}

int main() {
	//PROCESS INIT
	MemoryManager MM;
	PROTEZA proteza;
	MM.start();

	proteza.process_list.push_back(new PROTEZA::PCB(0, 0, "INIT", MM.createPageTable(16, 0)));
	running = proteza.process_list[0];

//	MM.LoadProgram("program.txt", 48, 2);
//	proteza.process_list.push_back(new PROTEZA::PCB(2, 0, "p2", MM.createPageTable(48, 2)));
//	running = proteza.process_list[1];
//	proteza.removeProcess(2,MM);
//	if(running != nullptr)

	/////////////MENU//////////////
	char ster = 'n';
	std::cout << "1. Stworz proces." << std::endl;
	std::cout << "2. Pokaz pamiec." << std::endl;
	std::cout << "3. Zmiana procesu running." << std::endl;
	std::cout << "4. Wyswietl proces running." << std::endl;
	std::cout << "5. Wyswietl tablice stronic procesu." << std::endl;
	std::cout << "6. Usun proces." << std::endl;
	std::cout << "7. Wyswietl plik stronicowania." << std::endl;
	std::cout << "8. Wpisanie danych do pamieci." << std::endl;
	std::cout << "9. Pobranie z pamieci od adresu do spacji." << std::endl;
	std::cout << "A. Wyswietl liste LRU." << std::endl;
	std::cout << "B. GO." << std::endl;
	/////////////MENU//////////////

	while (ster != 'k') {
		std::cin >> ster;
		
		if (ster == '1') {
			int Pid;
			int mem;
			std::string name;
			std::string path;

			std::cout << "Podaj PID: ";
			std::cin >> Pid;
			std::cout << "Podaj ilosc pamieci: ";
			std::cin >> mem;
			std::cout << "Podaj nazwe procesu: ";
			std::cin >> name;
			std::cout << "Podaj nazwe programu: ";
			std::cin >> path;

			if (proteza.checkPID(Pid) == false)
				continue;

			if (MM.LoadProgram(path, mem, Pid) == -1)
				continue;

			proteza.process_list.push_back(new PROTEZA::PCB(Pid, 0, name, MM.createPageTable(mem, Pid)));
			std::cout << "Utworzono proces " << name << std::endl;
		}
		if (ster == '2') {
			MM.showPMemory();
		}
		if (ster == '3') {
			int Pid;
			std::cout << "Podaj PID nowego running: ";
			std::cin >> Pid;
			proteza.selectRunning(Pid);
		}
		if (ster == '4') {

			proteza.showRunning();
		}
		if (ster == '5') {
			if (running == nullptr) {
				std::cout << "Nie ma procesu running" << std::endl;
				continue;
			}
			MM.showPageTable(running->page_table);
		}
		if (ster == '6') {
			int Pid;
			std::cout << "Podaj PID usuwanego procesu";
			std::cin >> Pid;
			MM.Remove(Pid);
			proteza.removeProcess(Pid,MM);
		}
		if (ster == '7') {
			MM.ShowPageFile();
		}
		if (ster == '8') {
			if (running == nullptr) {
				std::cout << "Nie ma procesu running" << std::endl;
				continue;
			}
			int adress;
			int Pid;
			std::string dane;
			char znak;
			//std::cout << "Podaj PID: ";
		//	std::cin >> Pid;
			std::cout << "Podaj adres: ";
			std::cin >> adress;
			std::cout << "Podaj dane do wpisania ";
			std::cin >> dane;
			MM.wr(adress,dane);
		}
		if (ster == '9') {
			int adress;
			std::cout << "Podaj adres: ";
			std::cin >> adress;
			std::cout << MM.Get(adress) << std::endl;
		}
		if (ster == 'A') {
			MM.ShowLRUList();
			std::cout << std::endl;
		}
		if (ster == 'B') {
			proteza.GO(MM);
		}
	}


	system("PAUSE");
}
