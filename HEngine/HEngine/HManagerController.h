#pragma once

using namespace std;

template<typename Manager, typename ObjectContainer>
class HManagerController
{
private:
	Manager* m_pManager;				//manager class
	ObjectContainer* m_pObjectContainer;//stl container
	void* m_pMe;
	void (Manager::*m_afterDeleteProcess)(void) = nullptr;
public:
	HManagerController(Manager* pManager, ObjectContainer* pObjectContainer, void* pMe):
		m_pManager(pManager), m_pObjectContainer(pObjectContainer), m_pMe(pMe) {}
	HManagerController(){}

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
