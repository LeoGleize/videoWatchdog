/*
 * cardexceptions.cpp
 *
 *  Created on: Oct 20, 2015
 *      Author: fcaldas
 */
#include "cardexceptions.h"

CardException::CardException(const char *message){
	this->msg_= message;
}

CardException::CardException(const std::string & message){
	this->msg_ = message;
}

CardException::CardException(const std::string &message, exceptionType etype){
	this->exType = etype;
	this->msg_ = message;
}

CardException::~CardException(){

}

exceptionType CardException::getExceptionType() const{
	return this->exType;
}

const char* CardException::what() const throw(){
	switch(exType){
		case NO_INPUT_EXCEPTION:
			return "No video input found";
	}
	return "Unknown exception";
}
