#pragma once

using namespace std;

template<typename Manager, typename ObjectContainer>
class HManagerController_map
{
private:
	Manager* m_pManager;				//manager class
	ObjectContainer* m_pObjectContainer;//stl container
	void* m_pMe;
	void (Manager::*m_afterDeleteProcess)(void) = nullptr;
public:
	HManagerController_map(Manager* pManager, ObjectContainer* pObjectContainer, void* pMe):
		m_pManager(pManager), m_pObjectContainer(pObjectContainer), m_pMe(pMe) {}
	HManagerController_map(){}

	void SetAfterDeleteProcess(void(Manager::*afterDeleteProcess)())
	{m_afterDeleteProcess = afterDeleteProcess;}

	void DeleteMe()
	{
		//�ڽ��� �������� �ڽ��� ��������鵵 �������� ������ �������� ���� �Լ� ���ÿ�
		//�ʿ��� �������� ����
		void (Manager:: * pointerSave)(void) = m_afterDeleteProcess;
		Manager* managerSave = m_pManager;

		//�ڱ� �ڽ��� �����̳ʿ��� �����ش�. �� ���� ����������� ��������.
		m_pObjectContainer->erase(m_pMe);

		//������ ������� �ʿ��� ó���� �Ѵ�.
		if (pointerSave != nullptr)
		{
			(managerSave->*pointerSave)();
		}
	}

	ObjectContainer* GetContainer() { return m_pObjectContainer; }
};


template<typename Manager,  typename ObjectType>
class HManagerController_vector
{
private:
	Manager* m_pManager;										  //manager class
	std::vector<std::pair<void*, ObjectType>>* m_pObjectContainer;//stl container
	void* m_pMe;
	void (Manager::* m_afterDeleteProcess)(void) = nullptr;
public:
	HManagerController_vector(Manager* pManager, std::vector<std::pair<void*, ObjectType>>* pObjectContainer, void* pMe) :
		m_pManager(pManager), m_pObjectContainer(pObjectContainer), m_pMe(pMe)
	{
	
	}
	HManagerController_vector() {}

	void SetAfterDeleteProcess(void(Manager::* afterDeleteProcess)())
	{
		m_afterDeleteProcess = afterDeleteProcess;
	}

	void DeleteMe()
	{
		//�ڽ��� �������� �ڽ��� ��������鵵 �������� ������ �������� ���� �Լ� ���ÿ�
		//�ʿ��� �������� ����
		void (Manager:: * pointerSave)(void) = m_afterDeleteProcess;
		Manager* managerSave = m_pManager;

		//�ڱ� �ڽ��� �����̳ʿ��� �����ش�. �� ���� ����������� ��������.

		for (typename std::vector<std::pair<void*, ObjectType>>::iterator it = m_pObjectContainer->begin();
			it != m_pObjectContainer->end();)
		{
			if (it->first == m_pMe)
			{
				m_pObjectContainer->erase(it);
				break;
			}
			else
			{
				it++;
			}
		}

		//������ ������� �ʿ��� ó���� �Ѵ�.
		if (pointerSave != nullptr)
		{
			(managerSave->*pointerSave)();
		}
	}

	std::vector<std::pair<void*, ObjectType>>* GetContainer() { return m_pObjectContainer; }
};