/*
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "DylibBundler.h"
#include <iostream>
#include "Utils.h"
#include "Settings.h"
#include "Dependency.h"


std::vector<Dependency> deps;

void changeLibPathsOnFile(std::string file_to_fix)
{
    std::cout << "\n* Fixing dependencies on " << file_to_fix.c_str() << std::endl;
    
    const int dep_amount = deps.size();
    for(int n=0; n<dep_amount; n++)
    {
        deps[n].fixFileThatDependsOnMe(file_to_fix);
    }
}

void addDependency(std::string path)
{
    if(path.compare(0, 9, "/usr/lib/") == 0) return;

    Dependency dep(path);
    
    // we need to check if this library was already added to avoid duplicates
    const int dep_amount = deps.size();
    for(int n=0; n<dep_amount; n++)
    {
        if(dep.mergeIfSameAs(deps[n])) return;
    }
    
    if(!Settings::isPrefixBundled(dep.getPrefix())) return;
    
    deps.push_back(dep);
}

/*
 *  Fill vector 'lines' with dependencies of given 'filename'
 */
void collectDependencies(std::string filename, std::vector<std::string>& lines)
{
    // execute "otool -L" on the given file and collect the command's output
    std::string cmd = "otool -L " + filename;
    std::string output = system_get_output(cmd);

    if(output.find("can't open file")!=std::string::npos or output.find("No such file")!=std::string::npos or output.size()<1)
    {
        std::cerr << "Cannot find file " << filename << " to read its dependencies" << std::endl;
        exit(1);
    }
    
    // split output
    tokenize(output, "\n", &lines);
}


void collectDependencies(std::string filename)
{
    std::vector<std::string> lines;
    collectDependencies(filename, lines);
       
    std::cout << "."; fflush(stdout);
    
    const int line_amount = lines.size();
    for(int n=0; n<line_amount; n++)
    {
        std::cout << "."; fflush(stdout);
        if(lines[n][0] != '\t') continue; // only lines beginning with a tab interest us
        else if (lines[n][1] == '@') {
            std::cout << "Skipping path relative to @executable_path"; fflush(stdout);
            continue;
        }
        
        addDependency( // trim useless info, keep only library name
                       lines[n].substr(1, lines[n].find(" (") )
                       );
    }
}
void collectSubDependencies()
{
    // print status to user
    int dep_amount = deps.size();
    
    // recursively collect each dependencie's dependencies
    while(true)
    {
        dep_amount = deps.size();
        for(int n=0; n<dep_amount; n++)
        {
            std::cout << "."; fflush(stdout);
            std::vector<std::string> lines;
            collectDependencies(deps[n].getOriginalPath(), lines);
            
            const int line_amount = lines.size();
            for(int n=0; n<line_amount; n++)
            {
                if(lines[n][0] != '\t') continue; // only lines beginning with a tab interest us
                
                addDependency( // trim useless info, keep only library name
                               lines[n].substr(1, lines[n].find(" (") )
                               );
            }//next
        }//next
        
        if(deps.size() == dep_amount) break; // no more dependencies were added on this iteration, stop searching
    }
}

void createDestDir()
{
    std::string dest_folder = Settings::destFolder();
    std::cout << "* Checking output directory " << dest_folder.c_str() << std::endl;
	
	// ----------- check dest folder stuff ----------
	bool dest_exists = fileExists(dest_folder);
	
	if(dest_exists and Settings::canOverwriteDir())
	{
        std::cout << "* Erasing old output directory " << dest_folder.c_str() << std::endl;
        std::string command = std::string("rm -r ") + dest_folder;
		if( systemp( command ) != 0)
		{
            std::cerr << "\n\nError : An error occured while attempting to override dest folder." << std::endl;
			exit(1);
		}
		dest_exists = false;
	}
	
	if(!dest_exists)
	{
		
		if(Settings::canCreateDir())
		{
            std::cout << "* Creating output directory " << dest_folder.c_str() << std::endl;
            std::string command = std::string("mkdir -p ") + dest_folder;
			if( systemp( command ) != 0)
			{
                std::cerr << "\n\nError : An error occured while creating dest folder." << std::endl;
				exit(1);
			}
		}
		else
		{
            std::cerr << "\n\nError : Dest folder does not exist. Create it or pass the appropriate flag for automatic dest dir creation." << std::endl;
			exit(1);
		}
	}
    
}

void doneWithDeps_go()
{
    std::cout << std::endl;
    const int dep_amount = deps.size();
    // print info to user
    for(int n=0; n<dep_amount; n++)
    {
        deps[n].print();
    }
    std::cout << std::endl;
    
    // copy files if requested by user
    if(Settings::bundleLibs())
    {
        createDestDir();
        
        for(int n=0; n<dep_amount; n++)
        {
            deps[n].copyYourself();
            changeLibPathsOnFile(deps[n].getInstallPath());
        }
    }
    
    const int fileToFixAmount = Settings::fileToFixAmount();
    for(int n=0; n<fileToFixAmount; n++)
    {
        changeLibPathsOnFile(Settings::fileToFix(n));
    }
}
