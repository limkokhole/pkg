#include <sys/types.h>

#include <err.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sysexits.h>
#include <unistd.h>

#include <pkg.h>

#include "install.h"

void
usage_install(void)
{
	fprintf(stderr, "usage: pkg install <pkg-name>\n");
	fprintf(stderr, "For more information see 'pkg help install'.\n");
}

int
exec_install(int argc, char **argv)
{
	struct pkg *pkg = NULL;
	struct pkgdb *db = NULL;
	struct pkg_jobs *jobs = NULL;
	int retcode = EPKG_OK;
	int i;

	if (argc < 2) {
		usage_install();
		return (-1);
	}

	if (geteuid() != 0) {
		warnx("installing packages can only be done as root");
		return (EX_NOPERM);
	}

	if (pkgdb_open(&db, PKGDB_REMOTE) != EPKG_OK) {
		pkg_error_warn("can not open database");
		retcode = EPKG_FATAL;
		goto cleanup;
	}

	if (pkg_jobs_new(&jobs, db) != EPKG_OK) {
		pkg_error_warn("pkg_jobs_new()");
		retcode = EPKG_FATAL;
		goto cleanup;
	}

	for (i = 1; i < argc; i++) {
		if ((pkg = pkgdb_query_remote(db, argv[i])) == NULL) {
			retcode = pkg_error_number();
			pkg_error_warn("can query the database");
			goto cleanup;
		}

		pkg_jobs_add(jobs, pkg);
	}

	/* print a summary before applying the jobs */
	pkg = NULL;
	printf("The following packages will be installed:\n");
	while (pkg_jobs(jobs, &pkg) == EPKG_OK) {
		printf("%s-%s\n", pkg_get(pkg, PKG_NAME), pkg_get(pkg, PKG_VERSION));
	}

	if (pkg_jobs_apply(jobs) != EPKG_OK)
		pkg_error_warn("can not install");

	cleanup:
	pkgdb_close(db);
	pkg_jobs_free(jobs);

	return (retcode == EPKG_OK ? 0 : 1);
}

