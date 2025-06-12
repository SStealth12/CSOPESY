#ifndef SCREEN_H
#define SCREEN_H

#include <string>


class Screen
{
private:
	std::string name_;
	size_t currentLine_;
	size_t totalLines_;
	std::string createTimestamp_;

	std::string getCurrentTimeStamp();

public:
	Screen(const std::string& name, size_t totalLines = 100);

	void draw() const;

	const std::string& getName() const { return name_; }

};

#endif // SCREEN_H