#ifndef IMPORTOPJ_H
#define IMPORTOPJ_H

#include "application.h"

class ApplicationWindow;
class OPJFile;

class ImportOPJ
{
public:
	ImportOPJ(ApplicationWindow *mw, const QString& filename);

	bool importTables(OPJFile opj);
	int error(){return parse_error;};

private:
	int parse_error;
	ApplicationWindow *mw;
};

#endif //IMPORTOPJ_H
