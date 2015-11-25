geometrydir = @datadir@/$(PACKAGE)/geometry

dist_noinst_DATA = 0000000000000000.xml

install-data-local:
	i=0; while [ $$i -lt 256 ] ; do subdir=$$(printf "%02x" $$i) ; dir="$(geometrydir)/$$subdir" ; if [ ! -d $$dir ] ; then mkdir $$dir ; fi ; if stat *$$subdir.xml >/dev/null 2>&1 ; then for file in *$$subdir.xml ; do $(install_sh_DATA) $$file $$dir ; done ; fi ; i=$$(expr $$i + 1) ; done
