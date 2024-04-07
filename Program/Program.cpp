﻿#include <iostream>
#include <stack>
#include <deque>
#include <vector>
#include <regex>
#include <string>
#include <fstream>
#include <algorithm>
#include <functional>
#include <numeric>
#include <map>
#include <filesystem>
#include <windows.h>
#include <typeinfo>
using namespace std;

using deque_iter = deque<string>::const_iterator;

class Notification {
private:
	static void changeConsoleColor(int colorCode = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) {
		HANDLE consoleHandle = GetStdHandle(STD_OUTPUT_HANDLE);
		SetConsoleTextAttribute(consoleHandle, colorCode);
	}

	static void printTextWithSpecificColor(int colorCode, string text) {
		changeConsoleColor(colorCode);
		cout << "\n" << text << "\n\n";
		changeConsoleColor();
	}

public:
	static void successNotification(string text) {
		printTextWithSpecificColor(10, "Успіх: " + text);
	}

	static void errorNotification(string text) {
		printTextWithSpecificColor(12, "Помилка: " + text);
	}
};

class Clipboard {
private:
	stack<string> clipboard;

public:
	void addData(string data) {
		clipboard.push(data);
	}

	deque_iter begin() {
		return clipboard._Get_container().begin();
	}

	deque_iter end() {
		return clipboard._Get_container().end();
	}

	int size() {
		return clipboard.size();
	}

	void deleteLastData() {
		clipboard.pop();
	}

	bool isEmpty() {
		return clipboard.size() == 0;
	}

	void printClipboard() {
		for (int i = 0; i < clipboard.size(); i++)
			cout << "\n" << i + 1 << ") " << clipboard._Get_container()[i];
		cout << endl;
	}
};

class Session;
class SessionsHistory;

class Editor {
private:
	friend class Program;

	static SessionsHistory* sessionsHistory;
	static Session* currentSession;
	static string* currentText;

	void tryToLoadSessions();
	void tryToUnloadSessions();

public:
	Editor();

	~Editor() {
		delete sessionsHistory;
		delete currentText;
	}

	void copy(string textToProcess, int startPosition, int endPosition);
	void paste(string* textToProcess, string textToPaste, int startPosition, int endPosition);
	void cut(string* textToProcess, int startPosition, int endPosition);
	void remove(string* textToProcess, int startPosition, int endPosition);

	static void setCurrentSession(Session* currentSession);
	static Session* getCurrentSession();
	static SessionsHistory* getSessionsHistory();
	static void setCurrentText(string* text);
	static string* getCurrentText();
	static void printCurrentText();
};

enum class TypesOfCommands {
	Copy,
	Paste,
	Cut,
	Delete,
	Undo,
	Redo
};

class Command {
protected:
	friend class FilesManager;

	Editor* editor;
	int startPosition, endPosition;
	string textToProcess, textToPaste;
	Command* previousCommand, *commandToUndo, *commandToRedo;
public:
	virtual void execute() = 0;
	virtual void undo() = 0;
	virtual Command* copy() = 0;

	void setEditor(Editor* editor) {
		this->editor = editor;
	}

	string getTextToProcess() {
		return textToProcess;
	}

	void setParameters(TypesOfCommands typeOfCommand, Command* previousCommand, Command* commandToUndo = nullptr, Command* commandToRedo = nullptr, int startPosition = 0, int endPosition = 0, string textToPaste = "") {
		if (typeOfCommand == TypesOfCommands::Undo)
		{
			this->commandToRedo = commandToRedo;
			return;
		}
		else if (typeOfCommand == TypesOfCommands::Redo) {
			this->commandToRedo = commandToRedo;
			return;
		}

		if (startPosition > endPosition)
			swap(startPosition, endPosition);
		if (endPosition > editor->getCurrentText()->size() - 1)
			endPosition = editor->getCurrentText()->size() - 1;

		this->startPosition = startPosition;
		this->endPosition = endPosition;
		this->textToProcess = *(editor->getCurrentText());
		this->previousCommand = previousCommand;

		if (typeOfCommand == TypesOfCommands::Paste)
			this->textToPaste = textToPaste;
	}
};

class Session {
private:
	int currentCommandIndexInHistory;
	stack<Command*, vector<Command*>> commandsHistory;
	Clipboard* clipboard;
	string name;

public:
	Session() { 
		clipboard = new Clipboard(); 
		currentCommandIndexInHistory = -1;
	}

	~Session() {
		while (!commandsHistory.empty()) {
			if (commandsHistory.top())
				delete commandsHistory.top();
			commandsHistory.pop();
		}
		delete clipboard;
	}

	string getName() {
		return name;
	}

	bool setName(string filename) {
		string forbiddenCharacters = "/\\\":?*|<>";

		for (char ch : forbiddenCharacters)
			if (filename.find(ch) != string::npos)
				return false;

		name = filename + ".txt";

		return true;
	}

	void addCommandAsLast(Command* command) {
		commandsHistory.push(command);
	}

	void deleteLastCommand() {
		delete commandsHistory.top();
		commandsHistory.pop();
	}

	void deleteCommandByIndex(int index) {
		auto ptrOnUnderlyingContainer = &commandsHistory._Get_container();
		auto ptrOnRetiringCommand = (*ptrOnUnderlyingContainer)[index];

		vector<Command*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringCommand);

		commandsHistory = stack<Command*, vector<Command*>>(notRetiringElements);

		delete ptrOnRetiringCommand;
	}

	Command* getLastCommand() {
		return commandsHistory.top();
	}

	Command* getCommandByIndex(int index) {
		return commandsHistory._Get_container()[index];
	}

	bool isCommandsHistoryEmpty() {
		return commandsHistory.size() == 0;
	}

	int sizeOfCommandsHistory() {
		return commandsHistory.size();
	}

	void printCommandsHistory();

	Clipboard* getClipboard() {
		return clipboard;
	}

	int getCurrentCommandIndexInHistory() {
		return currentCommandIndexInHistory;
	}

	void setCurrentCommandIndexInHistory(int currentCommandIndexInHistory) {
		this->currentCommandIndexInHistory = currentCommandIndexInHistory;
	}
};

class SessionsHistory {
private:
	stack<Session*, vector<Session*>> sessions;

public:
	~SessionsHistory() {
		while (!sessions.empty()) {
			delete sessions.top();
			sessions.pop();
		}
	}

	void addSessionToEnd(Session* session) {
		sessions.push(session);
	}

	Session* getLastSession() {
		return sessions.top();
	}

	Session* getFirstSession() {
		return sessions._Get_container().front();
	}

	Session* getSessionByIndex(int index) {
		return sessions._Get_container()[index];
	}

	string deleteLastSession() {
		string filename = sessions.top()->getName();

		delete sessions.top();
		sessions.pop();

		return filename;
	}

	string deleteFirstSession() {
		return deleteSessionByIndex(0);
	}

	string deleteSessionByIndex(int index) {
		auto ptrOnUnderlyingContainer = &sessions._Get_container();
		auto ptrOnRetiringSession = (*ptrOnUnderlyingContainer)[index];

		string filename = ptrOnRetiringSession->getName();

		vector<Session*> notRetiringElements;
		remove_copy(ptrOnUnderlyingContainer->begin(), ptrOnUnderlyingContainer->end(),
			back_inserter(notRetiringElements), ptrOnRetiringSession);

		sessions = stack<Session*, vector<Session*>>(notRetiringElements);

		delete ptrOnRetiringSession;

		return filename;
	}

	bool isEmpty() {
		return sessions.size() == 0;
	}

	int size() {
		return sessions.size();
	}

	void printSessionsHistory() {
		for (int i = 0; i < sessions.size(); i++)
			cout << "\nСеанс #" << i + 1 << ": " << sessions._Get_container()[i]->getName();
		cout << endl;
	}
};

class CopyCommand : public Command {
public:
	CopyCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class DeleteCommand : public Command {
public:
	DeleteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class CutCommand : public Command {
public:
	CutCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class PasteCommand : public Command {
public:
	PasteCommand(Editor* editor);

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class UndoCommand : public Command {
public:
	~UndoCommand() {
		if (commandToUndo)
			delete (commandToUndo);
	}

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class RedoCommand : public Command {
public:
	~RedoCommand() {
		if (commandToRedo)
			delete (commandToRedo);
	}

	void execute() override;

	void undo() override;

	Command* copy() override;
};

class FilesManager {
private:
	friend class Editor;

	static const string METADATA_DIRECTORY,
		SESSIONS_METADATA_DIRECTORY,
		CLIPBOARD_METADATA_DIRECTORY,
		AVAILABLE_SESSIONS_FILE,
		AVAILABLE_SESSIONS_FULLPATH,
		SESSIONS_DIRECTORY;

	static stack<string> getFilepathsForMetadata(string directory) {
		stack<string> filesFromMetadataDirectory;
		for (const auto& entry : filesystem::directory_iterator(directory))
			if (entry.path().extension() == ".txt")
				filesFromMetadataDirectory.push(entry.path().string());

		return filesFromMetadataDirectory;
	}

	static void deleteMetadataForDeletedSessions(SessionsHistory* sessionsHistory, string directory) {
		if (!filesystem::exists(SESSIONS_DIRECTORY))
			return;

		stack<string> filesFromMetadataDirectory = getFilepathsForMetadata(directory);

		if (sessionsHistory->isEmpty()) {
			while (!filesFromMetadataDirectory.empty()) {
				remove(filesFromMetadataDirectory.top().c_str());
				filesFromMetadataDirectory.pop();
			}
		}
		else {
			bool isPartOfRealtimeSessions = false;
			for (int i = 0; i < filesFromMetadataDirectory.size(); i++)
			{
				for (int j = 0; j < sessionsHistory->size(); j++)
				{
					if (filesFromMetadataDirectory._Get_container()[i] == sessionsHistory->getSessionByIndex(j)->getName())
						isPartOfRealtimeSessions = true;
				}
				if (!isPartOfRealtimeSessions)
					remove(filesFromMetadataDirectory._Get_container()[i].c_str());
				isPartOfRealtimeSessions = false;
			}
		}
	}

	static void writeSessionsMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, SESSIONS_METADATA_DIRECTORY);

		string filename, typeOfCommand, delimiter = "---\n";
		Session* session;
		Command* command;

		if (!filesystem::exists(SESSIONS_METADATA_DIRECTORY))
			filesystem::create_directories(SESSIONS_METADATA_DIRECTORY);

		ofstream ofs_aval_session(AVAILABLE_SESSIONS_FULLPATH);

		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			session = sessionsHistory->getSessionByIndex(i);

			filename = session->getName();
			ofs_aval_session << filename << endl;

			ofstream ofs_session(SESSIONS_DIRECTORY  + filename);

			ofs_session << session->getName() << endl;
			ofs_session << session->sizeOfCommandsHistory() << endl;
			ofs_session << session->getCurrentCommandIndexInHistory() << endl;

			for (int j = 0; j < session->sizeOfCommandsHistory(); j++)
			{
				command = session->getCommandByIndex(j);

				if (string(typeid(*command).name()) == "class CopyCommand")
					typeOfCommand = "CopyCommand\n";
				else if (string(typeid(*command).name()) == "class PasteCommand")
					typeOfCommand = "PasteCommand\n";
				else if (string(typeid(*command).name()) == "class CutCommand")
					typeOfCommand = "CutCommand\n";
				else
					typeOfCommand = "DeleteCommand\n";

				ofs_session << typeOfCommand;

				if (string(typeid(*command).name()) == "class CopyCommand")
					continue;

				ofs_session << delimiter;

				ofs_session << command->textToProcess;

				if (command->textToProcess == "")
					ofs_session << endl;

				ofs_session << delimiter;

				if(typeOfCommand == "PasteCommand\n")

				{
					ofs_session << command->textToPaste;

					if (command->textToPaste == "")
						ofs_session << endl;

					ofs_session << delimiter;
				}

				ofs_session << command->startPosition << endl;

				ofs_session << command->endPosition << endl;
			}

			ofs_session.close();
		}

		ofs_aval_session.close();
	}

	static void readSessionsMetadata(SessionsHistory* sessionsHistory, Editor* editor) {

		ifstream ifs_available_sessions(AVAILABLE_SESSIONS_FULLPATH);

		if (!ifs_available_sessions) return;

		string line, typeOfCommand, text, delimiter = "---";
		int countOfCommands;
		Session* session;
		Command* command, * previousCommand;

		stack<string> available_sessions;

		while (getline(ifs_available_sessions, line))
			available_sessions.push(line);

		ifs_available_sessions.close();

		for (int i = 0; i < available_sessions.size(); i++)
		{
			ifstream ifs_session(SESSIONS_DIRECTORY + available_sessions._Get_container()[i]);

			getline(ifs_session, line);
			session = new Session();
			session->setName(line);

			getline(ifs_session, line);
			countOfCommands = stoi(line);

			getline(ifs_session, line);
			session->setCurrentCommandIndexInHistory(stoi(line));

			for (int j = 0; j < countOfCommands; j++)
			{
				getline(ifs_session, line);
				if (line == "CopyCommand")
				{
					command = new CopyCommand(editor);
					session->addCommandAsLast(command);
					continue;
				}
				else if (line == "CutCommand")
					command = new CutCommand(editor);
				else if (line == "PasteCommand")
					command = new PasteCommand(editor);
				else
					command = new DeleteCommand(editor);

				if (session->sizeOfCommandsHistory() == 1)
					command->previousCommand = session->getLastCommand();

				getline(ifs_session, line);
				while (getline(ifs_session, line)) {
					if (line == delimiter)
						break;
					text += line + "\n";
				}
				command->textToProcess = text;

				if(line == "PasteCommand")
				{
					text = "";
					while (getline(ifs_session, line)) {
						if (line == delimiter)
							break;
						text += line + "\n";
					}
					command->textToPaste = text;
				}

				getline(ifs_session, line);
				command->startPosition = stoi(line);

				getline(ifs_session, line);
				command->endPosition = stoi(line);

				session->addCommandAsLast(command);
			}

			sessionsHistory->addSessionToEnd(session);

			ifs_session.close();
		}
	}

	static void writeClipboardMetadata(SessionsHistory* sessionsHistory) {
		deleteMetadataForDeletedSessions(sessionsHistory, CLIPBOARD_METADATA_DIRECTORY); // отдельно сделать удаление данных для clipboard, или просто взглянуть на параметры...

		string delimiter = "---\n";
		Clipboard* clipboard;
		deque_iter element, end;

		if (!filesystem::exists(CLIPBOARD_METADATA_DIRECTORY))
			filesystem::create_directories(CLIPBOARD_METADATA_DIRECTORY);


		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			ofstream ofs(CLIPBOARD_METADATA_DIRECTORY  + sessionsHistory->getSessionByIndex(i)->getName());

			clipboard = sessionsHistory->getSessionByIndex(i)->getClipboard();
			element = clipboard->begin();
			end = clipboard->end();

			for (element; element != end; ++element)
			{
				ofs << *element << endl;
				ofs << delimiter;
			}

			ofs.close();
		}
	}

	static void readClipboardMetadata(SessionsHistory* sessionsHistory) {
		Clipboard* clipboard;
		string data, text;
		for (int i = 0; i < sessionsHistory->size(); i++)
		{
			ifstream ifs(CLIPBOARD_METADATA_DIRECTORY  + sessionsHistory->getSessionByIndex(i)->getName());

			clipboard = sessionsHistory->getSessionByIndex(i)->getClipboard();
			while (getline(ifs, data))
			{
				if (data == "---")
				{
					text = text.substr(0, text.size() - 1);
					clipboard->addData(text);
					text = "";
				}
				else
					text += data + "\n";
			}

			ifs.close();
		}
	}

public:
	static string getSessionsDirectory() {
		return SESSIONS_DIRECTORY;
	}

	static string readFullDataFromFile(string fullFilepath) {
		string text, line;

		ifstream file(fullFilepath);

		while (getline(file, line))
			text += line + "\n";

		file.close();

		return text;
	}

	static bool overwriteDataInExistingFile(string fullFilepath, string newData) {
		ofstream file(fullFilepath);

		if (!file.is_open())
			return false;

		file << newData;
		file.close();

		return true;
	}
};

const string FilesManager::METADATA_DIRECTORY = "Metadata\\",
FilesManager::SESSIONS_METADATA_DIRECTORY = METADATA_DIRECTORY  + "Sessions\\",
FilesManager::CLIPBOARD_METADATA_DIRECTORY = METADATA_DIRECTORY  + "Clipboard\\",
FilesManager::AVAILABLE_SESSIONS_FILE = "Available_Sessions.txt",
FilesManager::AVAILABLE_SESSIONS_FULLPATH = METADATA_DIRECTORY + AVAILABLE_SESSIONS_FILE,
FilesManager::SESSIONS_DIRECTORY = "Sessions\\";

void Editor::tryToLoadSessions() {
	FilesManager::readSessionsMetadata(sessionsHistory, this);
	FilesManager::readClipboardMetadata(sessionsHistory);
}

void Editor::tryToUnloadSessions() {
	FilesManager::writeClipboardMetadata(sessionsHistory);
	FilesManager::writeSessionsMetadata(sessionsHistory);
}

Editor::Editor() {
	this->sessionsHistory = new SessionsHistory();
	currentText = new string("");
}

void Editor::copy(string textToProcess, int startPosition, int endPosition) {
	this->currentSession->getClipboard()->addData(textToProcess.substr(startPosition, endPosition - startPosition + 1));
}

void Editor::paste(string* textToProcess, string textToPaste, int startPosition, int endPosition) {
	if (startPosition == endPosition)
		(*textToProcess).insert(startPosition, textToPaste);
	else
		(*textToProcess).replace(startPosition, endPosition - startPosition + 1, textToPaste);
	*currentText = *textToProcess;
}

void Editor::cut(string* textToProcess, int startPosition, int endPosition) {
	copy(*textToProcess, startPosition, endPosition);
	remove(textToProcess, startPosition, endPosition);
	*currentText = *textToProcess;
}

void Editor::remove(string* textToProcess, int startPosition, int endPosition) {
	(*textToProcess).erase(startPosition, endPosition - startPosition + 1);
	*currentText = *textToProcess;
}

void Editor::setCurrentSession(Session* currentSession) {
	currentSession = currentSession;
}

Session* Editor::getCurrentSession() {
	return currentSession;
}

SessionsHistory* Editor::getSessionsHistory() {
	return sessionsHistory;
}

void Editor::setCurrentText(string* text) {
	currentText = text;
}

string* Editor::getCurrentText() {
	return currentText;
}

void Editor::printCurrentText() {
	cout << "\nЗміст файлу " << currentSession->getName() << ":\n";
	cout << *currentText << endl;
}

SessionsHistory* Editor::sessionsHistory;
Session* Editor::currentSession;
string* Editor::currentText;

CopyCommand::CopyCommand(Editor* editor) {
	this->editor = editor;
}

void CopyCommand::execute() {
	editor->copy(textToProcess, startPosition, endPosition);
}

void CopyCommand::undo() {
	Editor::getCurrentSession()->getClipboard()->deleteLastData();
}

Command* CopyCommand::copy() {
	return new CopyCommand(*this);
}

DeleteCommand::DeleteCommand(Editor* editor) {
	this->editor = editor;
}

void DeleteCommand::execute() {
	editor->remove(&textToProcess, startPosition, endPosition);
}

void DeleteCommand::undo() {
	*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
}

Command* DeleteCommand::copy() {
	return new DeleteCommand(*this);
}

CutCommand::CutCommand(Editor* editor) {
	this->editor = editor;
}

void CutCommand::execute() {
	editor->cut(&textToProcess, startPosition, endPosition);
}

void CutCommand::undo() {
	*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
	Editor::getCurrentSession()->getClipboard()->deleteLastData();
}

Command* CutCommand::copy() {
	return new CutCommand(*this);
}

PasteCommand::PasteCommand(Editor* editor) {
	this->editor = editor;
}

void PasteCommand::execute() {
	editor->paste(&textToProcess, textToPaste, startPosition, endPosition);
}

void PasteCommand::undo() {
	*(Editor::getCurrentText()) = previousCommand->getTextToProcess();
}

Command* PasteCommand::copy() {
	return new PasteCommand(*this);
}

void UndoCommand::execute() {
	commandToUndo->undo();
}

void UndoCommand::undo() { }

Command* UndoCommand::copy() { return nullptr; }

void RedoCommand::execute() {
	commandToUndo->execute();
}

void RedoCommand::undo() { }

Command* RedoCommand::copy() { return nullptr; }

void Session::printCommandsHistory() {
	if (isCommandsHistoryEmpty()) {
		Notification::errorNotification("за ций сеанс ще не було виконано жодної команди!");
		return;
	}
	for (int i = 0; i < commandsHistory.size(); i++)
	{
		cout << "\n" << i + 1 << ") ";
		if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class UndoCommand")
			cout << "UndoCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class CopyCommand")
			cout << "CopyCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class PasteCommand")
			cout << "PasteCommand\n";
		else if (string(typeid(*(commandsHistory._Get_container()[i])).name()) == "class CutCommand")
			cout << "CutCommand\n";
		else
			cout << "DeleteCommand\n";
	}
	cout << endl;
}

class CommandsManager {
private:
	map<TypesOfCommands, Command*> manager;

	void setParametersForCommand(TypesOfCommands typeOfCommand, int startPosition = 0, int endPosition = 0, string textToPaste = "") {
		Command* commandToUndo = nullptr, * commandToRedo = nullptr, * previousCommand = nullptr;

		if (typeOfCommand == TypesOfCommands::Undo)
			commandToUndo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurrentCommandIndexInHistory());

		if(typeOfCommand == TypesOfCommands::Redo)
			commandToRedo = Editor::getCurrentSession()->getCommandByIndex(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() + 1);

		if (Editor::getCurrentSession()->getCurrentCommandIndexInHistory() != -1 && typeOfCommand == TypesOfCommands::Undo && typeOfCommand == TypesOfCommands::Redo)
			previousCommand = Editor::getCurrentSession()->getLastCommand();

		manager[typeOfCommand]->setParameters(typeOfCommand, commandToUndo, commandToRedo, previousCommand, startPosition, endPosition, textToPaste);
	}

	void deleteUselessLeftCommandsIfNecessary(TypesOfCommands typeOfCommand) {
		if (typeOfCommand != TypesOfCommands::Undo && typeOfCommand != TypesOfCommands::Redo)
			if (isThereAnyCommandForward()) {
				int countOfCommandsToDelete = Editor::getCurrentSession()->sizeOfCommandsHistory() - 1 - Editor::getCurrentSession()->getCurrentCommandIndexInHistory();
				for (int i = 1; i < countOfCommandsToDelete; i++)
					Editor::getCurrentSession()->deleteLastCommand();
			}
	}

public:
	CommandsManager(Editor* editor) {
		manager[TypesOfCommands::Copy] = new CopyCommand(editor);
		manager[TypesOfCommands::Paste] = new PasteCommand(editor);
		manager[TypesOfCommands::Cut] = new CutCommand(editor);
		manager[TypesOfCommands::Delete] = new DeleteCommand(editor);
		manager[TypesOfCommands::Undo] = new UndoCommand();
		manager[TypesOfCommands::Redo] = new RedoCommand();
	}

	~CommandsManager() {
		for (auto& pair : manager) {
			delete (pair.second);
		}
		manager.clear();
	}

	bool isThereAnyCommandForward() {
		return !Editor::getCurrentSession()->isCommandsHistoryEmpty() &&
			Editor::getCurrentSession()->getCurrentCommandIndexInHistory() < Editor::getCurrentSession()->sizeOfCommandsHistory() - 1;
	}

	void invokeCommand(TypesOfCommands typeOfCommand, int startPosition = 0, int endPosition = 0, string textToPaste = "") {

		deleteUselessLeftCommandsIfNecessary(typeOfCommand);
		setParametersForCommand(typeOfCommand, startPosition, endPosition, textToPaste);

		manager[typeOfCommand]->execute();

		if (typeOfCommand != TypesOfCommands::Undo && typeOfCommand != TypesOfCommands::Redo)
			Editor::getCurrentSession()->addCommandAsLast(manager[typeOfCommand]->copy());

		if (typeOfCommand != TypesOfCommands::Undo)
			Editor::getCurrentSession()->setCurrentCommandIndexInHistory(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() + 1);
		else
			Editor::getCurrentSession()->setCurrentCommandIndexInHistory(Editor::getCurrentSession()->getCurrentCommandIndexInHistory() - 1);
	}
};

class Program {
private:
	Editor* editor;
	CommandsManager* commandsManager;

	bool validateEnteredNumber(string option, short firstOption, short lastOption) {
		if (option.empty())
			return false;

		for (char num : option)
			if (num < '0' || num > '9')
				return false;

		auto convertedValue = stoull(option);

		return firstOption <= convertedValue && convertedValue <= lastOption;
	}

	int enterNumberInRange(string message, int firstOption, int lastOption, string error = "були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!") {
		bool isOptionVerified = false;
		string option;

		cout << "\n" << message;
		getline(cin, option);

		isOptionVerified = validateEnteredNumber(option, firstOption, lastOption);

		if (!isOptionVerified)
			Notification::errorNotification(error);

		return isOptionVerified? stoi(option) : -1;
	}

	void wayToGetTextForAddingMenu(int& choice) {
		cout << "\nЯк ви хочете додати текст:\n";
		cout << "0. Назад\n";
		cout << "1. Ввівши з клавіатури\n";
		cout << "2. З буферу обміну\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}

	string getTextUsingKeyboard() {
		string line, text;

		cout << "\nВведіть текст (зупинити - з наступного рядка введіть -1):\n";
		while (getline(cin, line)) {
			if (line == "-1")
				break;
			text += line + "\n";
		}

		return text;
	}

	void getTextFromClipboardMenu(int& choice) {
		cout << "\nЯкі дані бажаєте отримати з буферу обміну:\n";
		cout << "0. Назад\n";
		cout << "1. Останні\n";
		cout << "2. Найперші\n";
		cout << "3. За індексом\n";
		choice = enterNumberInRange("Ваш вибір: ", 1, 3);
	}

	string getTextFromClipboard() {
		auto clipboard = Editor::getCurrentSession()->getClipboard();
		if (clipboard->isEmpty()) {
			Notification::errorNotification("в буфері обміну ще немає даних!");
			return "";
		}

		int choice;

		clipboard->printClipboard();
		getTextFromClipboardMenu(choice);

		switch (choice)
		{
		case 0:
			cout << "\nПовернення до меню вибору способа додавання текста.\n\n";
			return "";
		case 1:
			return *(clipboard->end() - 1);
		case 2:
			return *(clipboard->begin());
		case 3:
			choice = enterNumberInRange("Введіть номер даних: ", 1, clipboard->end() - clipboard->begin());
			if(choice != -1)
				return *(clipboard->begin() + choice - 1);
			return "";
		}
	}

	bool executeGettingTextForAdding(string& text) {
		int choice;

		do
		{
			Editor::printCurrentText();
			wayToGetTextForAddingMenu(choice);

			switch (choice)
			{
			case 0:
				cout << "\nПовернення до Меню дій над змістом.\n\n";
				return false;
			case 1:
				text = getTextUsingKeyboard();
				return true;
			case 2:
				text = getTextFromClipboard();
				return true;
			}
		} while (true);
	}

	void wayToPasteTextMenu(int& choice) {
		cout << "\nЯк ви хочете вставити текст:\n";
		cout << "0. Вийти в Меню дій над змістом\n";
		cout << "1. В кінець\n";
		cout << "2. На початок\n";
		cout << "3. За індексом\n";
		cout << "4. Замінивши певну частину в файлі\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 4);
	}

	void pasteText(int startIndex, int endIndex, string textToPaste) {
		commandsManager->invokeCommand(TypesOfCommands::Paste, startIndex, endIndex, textToPaste);
		Notification::successNotification("дані були успішно додані в файл!");
	}

	void pasteTextInBeginningOrEnd(int choice, string textToPaste) {
		if (Editor::getCurrentText()->size() == 0 || choice == 2)
			pasteText(0, 0, textToPaste);
		else if (choice == 1 && Editor::getCurrentText()->size() != 0)
			pasteText(Editor::getCurrentText()->size() - 1, Editor::getCurrentText()->size() - 1, textToPaste);
	}

	int pasteTextByIndex(string textToPaste) {
		int startOfRange = 0, endOfRange = 0;
		if (Editor::getCurrentText()->size() > 0)
			endOfRange = Editor::getCurrentText()->size() - 1;

		int index = enterNumberInRange("Введіть індекс, за яким будете вставляти текст", startOfRange, endOfRange);

		if (index == -1)
			return index;

		commandsManager->invokeCommand(TypesOfCommands::Paste, index, index, textToPaste);

		return index;
	}

	int pasteTextFromIndexToIndex(string textToPaste) {
		int startOfRange = 0, endOfRange = 0;
		if (Editor::getCurrentText()->size() > 0)
			endOfRange = Editor::getCurrentText()->size() - 1;

		int startIndex = enterNumberInRange("Введіть індекс, за яким будете вставляти текст", startOfRange, endOfRange);
		int endIndex = enterNumberInRange("Введіть індекс, за яким будете вставляти текст", startOfRange, endOfRange);

		if (startIndex == -1 || endIndex == -1)
			return -1;

		commandsManager->invokeCommand(TypesOfCommands::Paste, startIndex, endIndex, textToPaste);

		return endIndex;
	}

	bool executeAddingTextToFile() {
		string textToPaste;
		bool shallTextBeAdded = executeGettingTextForAdding(textToPaste);
		if (!shallTextBeAdded || textToPaste == "")
			return false;

		int choice;
		Editor::printCurrentText();
		wayToPasteTextMenu(choice);

		switch (choice) {
		case 0:
			cout << "\nПовернення до Меню дій над змістом.\n\n";
			return false;
		case 1:
		case 2:
			pasteTextInBeginningOrEnd(choice, textToPaste);
			return true;
		case 3:
			choice = pasteTextByIndex(textToPaste);
			if(choice != -1)
				return true;
			return false;
		case 4:
			choice = pasteTextFromIndexToIndex(textToPaste);
			if (choice != -1)
				return true;
			return false;
		}
	}

	void delCopyOrCutTextMenu(int& choice, string action) {
		cout << "\nСкільки хочете " + action + ":\n";
		cout << "0. Назад\n";
		cout << "1. Весь зміст\n";
		cout << "2. З певного по певний індекс\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 2);
	}

	bool delCopyOrCutWholeText(TypesOfCommands typeOfCommand, string actionInPast) {
		commandsManager->invokeCommand(typeOfCommand, 0, Editor::getCurrentText()->size() - 1);
		Notification::successNotification("дані були успішно " + actionInPast + "!");
		return true;
	}

	bool delCopyOrCutFromIndexToIndex(TypesOfCommands typeOfCommand, string actionInPast) {
		int startIndex, endIndex;

		startIndex = enterNumberInRange("Початковий індекс: ", 0, Editor::getCurrentText()->size() - 1, "був введений індекс, який виходить за межі тексту!");
		endIndex = enterNumberInRange("Кінцевий індекс: ", 0, Editor::getCurrentText()->size() - 1, "був введений індекс, який виходить за межі тексту!");

		if(startIndex != -1 && endIndex != -1)
		{
			commandsManager->invokeCommand(typeOfCommand, startIndex, endIndex);
			Notification::successNotification("дані були успішно " + actionInPast + "!");
			return true;
		}
		return false;
	}

	bool delCopyOrCutText(TypesOfCommands typeOfCommand, string actionForMenu, string actionInPast) {
		int choice;

		Editor::printCurrentText();
		delCopyOrCutTextMenu(choice, actionForMenu);

		switch (choice)
		{
		case 0:
			cout << "\nПовернення до Меню дій над змістом.\n\n";
			return false;
		case 1:
			return delCopyOrCutWholeText(typeOfCommand, actionInPast);
		case 2:
			return delCopyOrCutFromIndexToIndex(typeOfCommand, actionInPast);
		}
	}

	bool chooseRootDelCopyOrCut(string action) {
		if (Editor::getCurrentText()->size() == 0)
		{
			Notification::errorNotification("немає тексту, який можна було б " + action + "!");
			return false;
		}
		else
			if (action == "видалити")
				return delCopyOrCutText(TypesOfCommands::Delete, action, "видалені");
			else if (action == "скопіювати")
				return delCopyOrCutText(TypesOfCommands::Copy, action, "скопійовані");
			else
				return delCopyOrCutText(TypesOfCommands::Cut, action, "вирізані");
	}

	void makeActionsOnContentMenu(int& choice) {
		cout << "\nМеню дій над змістом:\n";
		cout << "0. Назад\n";
		cout << "1. Додати текст\n";
		cout << "2. Видалити текст\n";
		cout << "3. Копіювати текст\n";
		cout << "4. Вирізати текст\n";
		cout << "5. Скасувати команду\n";
		cout << "6. Повторити команду\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 6);
	}

	void readDataFromFile() {
		string textFromFile = FilesManager::readFullDataFromFile(FilesManager::getSessionsDirectory() + editor->currentSession->getName());
		commandsManager->invokeCommand(TypesOfCommands::Paste, 0, 0, textFromFile);
	}

	bool undoAction() {
		if (Editor::getCurrentSession()->sizeOfCommandsHistory() > 1)
			commandsManager->invokeCommand(TypesOfCommands::Undo);
		else
			Notification::errorNotification("немає дій, які можна було б скасувати!");
		return Editor::getCurrentSession()->sizeOfCommandsHistory() > 1;
	}

	bool redoAction() {
		bool isThereAnyCommandForward = commandsManager->isThereAnyCommandForward();
		if (isThereAnyCommandForward)
			commandsManager->invokeCommand(TypesOfCommands::Redo);
		else
			Notification::errorNotification("немає дій, які можна було б повторити!");
		return isThereAnyCommandForward;
	}

	void executeMakeActionsOnContentMenu() {
		commandsManager = new CommandsManager(editor);
		bool wasTextSuccessfullyChanged;
		int choice;

		readDataFromFile();

		do
		{
			wasTextSuccessfullyChanged = false;
			Editor::printCurrentText();
			makeActionsOnContentMenu(choice);

			switch (choice)
			{
			case 0:
				cout << "\nПовернення до Меню для отримання сеансу.\n\n";
				delete (commandsManager);
				return;
			case 1:
				wasTextSuccessfullyChanged = executeAddingTextToFile();
				continue;
			case 2:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("видалити");
				continue;
			case 3:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("скопіювати");
				continue;
			case 4:
				wasTextSuccessfullyChanged = chooseRootDelCopyOrCut("вирізати");
				continue;
			case 5:
				wasTextSuccessfullyChanged = undoAction();
				continue;
			case 6:
				wasTextSuccessfullyChanged = redoAction();
			}
			if(wasTextSuccessfullyChanged)
				FilesManager::overwriteDataInExistingFile(FilesManager::getSessionsDirectory() + Editor::getCurrentSession()->getName(), *(Editor::getCurrentText()));
		} while (true);
	}

	void templateForMenusAboutSessions(int& choice, string action) {
		cout << "\nМеню для " << action.substr(0, 6) + "ння" << " сеансу:\n";
		cout << "Який сеанс хочете " << action << ":\n";
		cout << "0. Назад\n";
		cout << "1. Останній\n";
		cout << "2. Найперший\n";
		cout << "3. За позицією\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 3);
	}

	void templateForExecutingMenusAboutSessions(function<void(int)> mainFunc, function<void(int&)> menu, function<void()> additionalFunc = nullptr) {
		int choice;
		do
		{
			Editor::getSessionsHistory()->printSessionsHistory();
			menu(choice);
			switch (choice)
			{
			case 0:
				cout << "\nПовернення до Головного меню.\n\n";
				return;
			case 1:
			case 2:
			case 3:
				if (doesAnySessionExist())
				{
					choice == 1 ? mainFunc(Editor::getSessionsHistory()->size()) :
						choice == 2 ? mainFunc(1) : mainFunc(-1);
					if(additionalFunc)
						additionalFunc();
				}
			}
		} while (true);
	}

	bool tryToEnterIndexForSession(int& index) {
		if (index == -1) {
			index = enterNumberInRange("Введіть номер сеансу: ", 1, Editor::getSessionsHistory()->size());
			if (index == -1)
			{
				Notification::errorNotification("були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!");
				return false;
			}
		}
		return true;
	}

	void printGettingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "отримати");
	}

	void setCurrentSessionByIndex(int index = -1) {
		if(tryToEnterIndexForSession(index))
			Editor::setCurrentSession(Editor::getSessionsHistory()->getSessionByIndex(index - 1));
	}

	void executeGettingSessionsMenu() {
		function<void(int)> mainFunc = [this](int index) {
			setCurrentSessionByIndex(index);
			};
		function<void(int&)> menu = [this](int& choice) {
			printGettingSessionsMenu(choice);
			};
		function<void()> additionalFunc = [this]() {
			executeMakeActionsOnContentMenu();
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu, additionalFunc);
	}

	void printDeletingSessionsMenu(int& choice) {
		templateForMenusAboutSessions(choice, "видалити");
	}

	void deleteSessionByIndex(int index = -1) {
		if(tryToEnterIndexForSession(index)){
			string nameOfSession = Editor::getSessionsHistory()->deleteSessionByIndex(index - 1);
			string pathToSession = FilesManager::getSessionsDirectory() + nameOfSession;
			remove(pathToSession.c_str());

			Notification::successNotification("сеанс був успішно видалений!");
		}
	}

	void executeDeletingSessionsMenu() {
		function<void(int)> mainFunc = [this](int index) {
			deleteSessionByIndex(index);
			};
		function<void(int&)> menu = [this](int& choice) {
			printDeletingSessionsMenu(choice);
			};

		templateForExecutingMenusAboutSessions(mainFunc, menu);
	}

	void printManagingSessionsMenu(int& choice) {
		cout << "\nГоловне меню:\n";
		cout << "0. Закрити програму\n";
		cout << "1. Довідка\n";
		cout << "2. Створити сеанс\n";
		cout << "3. Відкрити сеанс\n";
		cout << "4. Видалити сеанс\n";
		choice = enterNumberInRange("Ваш вибір: ", 0, 4);
	}

	void printReferenceInfo() {
		cout << "\nДовідка до програми:\n\n";
		cout << "Вашої уваги пропонується програму курсової роботи з об'єктно-орієнтованого програмування, побудована за допомогою патерну проектування \"Команда\".\n";
		cout << "За допомогою програми можна створювати, редагувати та видаляти текстові файли, використовуючи при цьому команди Копіювання, Вставка, Вирізання та Відміна операції.\n\n";
		cout << "Розробник: Бредун Денис Сергійович з групи ПЗ-21-1/9.\n\n";
	}

	void createSession() {
		Session* newSession = new Session();
		string filename;

		cout << "\nВведіть ім'я сеансу (заборонені символи: /\\\":?*|<>): ";
		getline(cin, filename);

		if (newSession->setName(filename))
		{
			ofstream file(FilesManager::getSessionsDirectory() + newSession->getName());
			file.close();
			Editor::getSessionsHistory()->addSessionToEnd(newSession);
			Notification::successNotification("сеанс був успішно створений!");
		}
		else
		{
			delete newSession;
			Notification::errorNotification("створення сеансу було припинено!");
		}
	}

	bool doesAnySessionExist() {
		if (Editor::getSessionsHistory()->isEmpty())
			Notification::errorNotification("в даний момент жодного сеансу немає!");
		return !Editor::getSessionsHistory()->isEmpty();
	}

public:

	void executeMainMenu() {
		int choice;
		editor = new Editor();
		editor->tryToLoadSessions(); //перевірю, коли буду переглядати клас FilesManager

		do
		{
			printManagingSessionsMenu(choice);
			switch (choice)
			{
			case 0:
				cout << "\nДо побачення!\n";
				editor->tryToUnloadSessions(); //перевірю, коли буду переглядати клас FilesManager
				delete editor;
				exit(0);
			case 1:
				printReferenceInfo();
				continue;
			case 2:
				createSession();
				continue;
			case 3:
			case 4:
				if (doesAnySessionExist())
					choice == 3 ? executeGettingSessionsMenu() : 
								  executeDeletingSessionsMenu();
			}

		} while (true);
	}
};

int main()
{
	SetConsoleCP(1251);
	SetConsoleOutputCP(1251);

	Program program;
	program.executeMainMenu();
}

//Заметки:

//виводити результат після кожної дії

//удалить лишнее

//удалить ненужные пункты в меню

//пользователь не должен вводить путь к файлам

//использовать относительный путь для ввода директорий

//путь по умолчанию

//я могу создавать файлы t1.ttxt

//если два файла имеют одинаковое имя, то сессия добавляется, а файл - нет

//доделать обработку истории команд и её откачку

//сделать, чтобы пользователь мог либо бесконечно отменять последнее действие, либо двигаться по истории комманд назад, ибо иначе записывать всю историю коммманд в файл и их данные - бессмысленно

//добавить команду обратную отмотке истории

//записывать в файл textToPaste

//запись с буфера обмена в файл - дерьмо

//при редактировании буфера обмена также надо учитывать команды Cut (но редактирование буфера обмена мы уберём)

//проверить запись и считывание метаданных нескольких файлов

//проверить все new на delete

//при вводе 0 происходит перемещение на предыдущий пункт меню, при этом об этом никак не уведомляется

//если в буфере обмена последние данные = тем, которые копируем в даннный момент, то не копируем их

//очищать консольное окно после операций?

//большое меню!

//выводить постоянно состояние файла и сессии!








//упорядочить методы по группам

//много где используется enterNumberInRange, хотя там надо просто верификация введённого числа

//два метода changeFileMetadata имеют одинаковую логику

// (зупинити ввід - -1) - це потрібно ввести на наступному рядку

//проверить функции на правильное разделение по логике

//сделать многие меню поо структуре как executeMakeActionsOnContentMenu

//мб убрать метод отдельный для сохранения методанных буфера обмена?

//при вводе двух индексов при вставке или удалении разные реакции на ошибки

//отцентрировать меню?

//вынести отдельно методы из классов

//разное поведение, если вводить огромное полож число или отриц число при валидации

//при попытке удалить сессии и вводе:
/*
Ваш вибір: info

Помилка: були введені зайві символи або число, яке виходить за межі набору цифр наданих варіантів!
*/

/*надо разделять валидацию вводимого числа и валидацию для меню*/

//убрать подобное: "class UndoCommand"

//норм ли:
//if (option == "exit")
//exit(1);
//			else if (option == "info")
//				return -1;
//норм ли case -1: в меню?

//убрать лищние библиотеки

//убрать 
//if (!doesAnySessionExist())
//continue; из множества мест

//вводить данные надо не в функции верификации ввода данных файла и меню!

//убрать длинные стрелки наподобие sessionsHistory->getSessionByIndex(i)->getClipboard()->

//мб разместить пункты менюх всех по функциям

//"параметри файлу", а не "системні"

//добавить заголовки как для страниц в пункты меню

//сделать многие методы в одном месте

//методы writeNewTextForContent и pasteDataFromClipboard имеют одинаковую логику

//классы команд наследуют ненужные вещи

//меню makeActionsOnContentMenu большое

//большие параметры передаются в методы

//ненужные заздалегідь оголошені класи - Command

//есть функции-болванки

//избавиться от лишних функций и классов

//фраза "Довідка:"

//вынести методы отдельно от классов

//меню действий над содержанием имеют одинаковые тектсты - можно вынести в отдельный метод












//доделать доп пункты из задания курсовой для меню

//возможно добавить предпросмотр при действиях над змістом

//обрабатывать закрытие пользователем проги, чтобы сохранять метаданные

//добавить историю действий для каждой сессии, как в гугл док

//идея вывода инфы с помощью html

//взять идеи у Стаса

//написать комменты к классам










//изменить лит джерела со "страниц" на "page"