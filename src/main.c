// TODO: Backup subcommand
// TODO: Restore subcommand

#include "./p2.h"

int main(const int argc, const char **argv)
{
	copt_program_init("p2", "0.2.0", "[OPTION] [ARGS...]");

	copt_add_option("HELP", "h", "help", "Print help message", "");
	copt_add_option("VERSION", "v", "version", "Print version", "");
	copt_add_option("LIST", "l", "list", "List passwords", "");
	copt_add_option("NEW", "n", "new", "Create a new password",
			"[NAME]");
	copt_add_option("DELETE", "d", "delete", "Delete a password",
			"[NAME]");
	copt_add_option("PRINT", "p", "print", "Print a password", "[NAME]");
	copt_add_option("COPY", "c", "copy", "Copy a password to clipboard",
			"[NAME]");
	copt_add_option("RENAME", "r", "rename", "Rename a password",
			"[NAME] [NEW NAME]");
	copt_add_option("BACKUP", "b", "backup",
			"Backup password directory into a .tar", "[OUTPUT]");
    copt_add_option("RESTORE", "rs", "restore", "Restore passwords from .tar backup", "[BACKUP FILE]");

    if (argc < 2) {
        printError("No subcommand given");
        return cmdHelp(argc, argv);
    } else if (copt_option_is("HELP", argc, argv)) {
        return cmdHelp(argc, argv);
    } else if (copt_option_is("VERSION", argc, argv)) {
        return cmdVersion(argc, argv);
    } else if (copt_option_is("LIST", argc, argv)) {
        return cmdList(argc, argv);
    } else if (copt_option_is("NEW", argc, argv)) {
        return cmdNew(argc, argv);
    } else if (copt_option_is("DELETE", argc, argv)) {
        return cmdDelete(argc, argv);
    } else if (copt_option_is("PRINT", argc, argv)) {
        return cmdPrint(argc, argv);
    } else if (copt_option_is("COPY", argc, argv)) {
        return cmdCopy(argc, argv);
    } else if (copt_option_is("RENAME", argc, argv)) {
        return cmdRename(argc, argv);
    } else if (copt_option_is("BACKUP", argc, argv)) {
        return cmdBackup(argc, argv);
    } else if (copt_option_is("RESTORE", argc, argv)) {
        //TODO
        // return cmdRestore(argc, argv);
    } else {
        printError("Unrecognised subcommand");
        return cmdHelp(argc, argv);
    }

}
