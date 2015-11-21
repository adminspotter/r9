texturedir = @datadir@/$(PACKAGE)/texture

dist_noinst_DATA = 0000000000000000.xml

install-data-local:
	i=0; while [ $$i -lt 256 ] ; do subdir=$$(printf "%02x" $$i); dir="$(texturedir)/$$subdir"; mkdir $$dir; for file in *$$subdir.xml ; do $(install_sh_DATA) $$file $$dir ; done ; i=$$(expr $$i + 1); done
