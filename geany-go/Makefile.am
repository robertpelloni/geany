EXTRA_DIST = go.mod go.sum main.go build/ config/ editor/ filetypes/ log/ macros/ nav/ project/ search/ symbols/ templates/ ui/ utils/

all-local:
	cd $(srcdir) && go build -buildmode=c-shared -o $(abs_builddir)/libgeanygo.so main.go
	if [ "$(abs_builddir)" != "$(srcdir)" ]; then cp $(abs_builddir)/libgeanygo.h $(srcdir)/libgeanygo.h || true; fi

clean-local:
	rm -f $(abs_builddir)/libgeanygo.so $(abs_builddir)/libgeanygo.h $(srcdir)/libgeanygo.h
