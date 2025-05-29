#pragma once
#include <unordered_set>

template <typename T>
class Sector
{
public:
	Sector()
	{
		//InitializeSRWLock(lock);
	}
	~Sector()
	{

	}
	int Enter(T p)
	{
		players.insert(p);
		p->currentSector = this;
		return 1;
	}

	int Leave(T p)
	{
		players.erase(p);
		p->currentSector = nullptr;
		return 1;
	}

	const std::unordered_set<T>* GetSectorPlayers() const
	{
		if (players.size() == 0)
		{
			return nullptr;
		}
		return &players;
	}
	//SRWLOCK lock;
private:
	std::unordered_set<T> players;

};