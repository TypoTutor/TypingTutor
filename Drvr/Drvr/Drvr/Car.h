#include <iostream>
#include <vector>
#include "CarPart.h"

#ifndef _CAR_
#define _CAR_

class Car
{
    private:
        //we makee the assumption that the first part will always be a standalone
        //and not have any other nodes, like a tree. Only 1 at the top, then navigate downwards
        //WE ALSO make the assumption that the min num of components is 3 and max is 14

        std::vector<int> hierarchyNumber;
        std::vector<CarPart> carParts; //this will consist of ALL the car parts
        std::vector<CarPart> currentParts; //this is the current level we are in
    public:
        void loadFile(std::string);
		Car();
        ~Car();

        //we use this function to check if there is need for zooming, if not 0, means there is child
        int getHierarchyNumber(int) const;
        //get the car part of the level we are currently in, we need to know what is the id of the part
        //we are currently at
        void setCurrentCarParts(std::vector<int>);
        std::vector<CarPart> getCurrentCarParts() const;

        //when the user clicks zoom in (up) or zoom out (down) in the program
        //we should probably check using the function getHierarchyNumber to determine if the user
        //is allowed to zoom in in the first place before using this function
        //then to get the parts of the same layer we use the getCurrentCarParts
        void zoomIn(int);
        void zoomOut(int);

        //text editor functions
        void textEditorMenu();
        void editByBatch();
        void editBatchPart(int); //somewhat similar to editSelectedPart but used for batch editing

        void editIndividually(); //user wants to modify one by one
        void editSelectedPart(int); //user selected what needs to be modified and goes to this function

        //function used to write to file, same function name with carPart
        void writeToFile();

        void outputFile(std::string);

		//reset function, to reset to its original state
		void resetFunc();
};

#endif // _CAR_
