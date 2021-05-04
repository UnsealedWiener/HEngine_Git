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
		//자신이 지워지면 자신의 멤버변수들도 지워지기 때문에 지워지기 전에 함수 스택에
		//필요한 변수들을 저장
		void (Manager:: * pointerSave)(void) = m_afterDeleteProcess;
		Manager* managerSave = m_pManager;

		//자기 자신을 컨테이너에서 지워준다. 이 순간 멤버변수들이 지워진다.
		m_pObjectContainer->erase(m_pMe);

		//저장한 변수들로 필요한 처리를 한다.
		if (pointerSave != nullptr)
		{
			(managerSave->*pointerSave)();
		}
	}

	ObjectContainer* GetContainer() { return m_pObjectContainer; }
};
