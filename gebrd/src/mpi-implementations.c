#include "mpi-implementations.h"

/*
 * OpenMPI definition
 */

static gchar * gebr_open_mpi_initialize(GebrdMpiInterface * mpi);
static gchar * gebr_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command);
static gchar * gebr_open_mpi_finalize(GebrdMpiInterface * mpi);

GebrdMpiInterface * gebr_open_mpi_new(const gchar * bin_path, const gchar * lib_path)
{
	GebrOpenMpi * self;
	GebrdMpiInterface * mpi;

	self = g_new(GebrOpenMpi, 1);
	mpi = (GebrdMpiInterface*)self;

	self->bin_path = bin_path;
	self->lib_path = lib_path;
	mpi->initialize = gebr_open_mpi_initialize;
	mpi->build_command = gebr_open_mpi_build_command;
	mpi->finalize = gebr_open_mpi_finalize;

	return mpi;
}

static gchar * gebr_open_mpi_initialize(GebrdMpiInterface * mpi)
{
	return NULL;
}

static gchar * gebr_open_mpi_build_command(GebrdMpiInterface * mpi, const gchar * command)
{
	GebrOpenMpi * self;
	GString * cmd;

	self = (GebrOpenMpi*)mpi;
	cmd = g_string_new(NULL);
	g_string_printf(cmd, "LD_LIBRARY_PATH=%s:$LD_LIBRARY_PATH PATH=%s:$PATH mpirun.openmpi -np %d %s",
			self->lib_path, self->bin_path, mpi->n_processes, command);
	return g_string_free(cmd, FALSE);
}

static gchar * gebr_open_mpi_finalize(GebrdMpiInterface * mpi)
{
	return NULL;
}
