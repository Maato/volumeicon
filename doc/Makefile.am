EXTRA_DIST = volumeicon.1.pod volumeicon.5.pod

dist_man_MANS = volumeicon.1 volumeicon.5

volumeicon.1: volumeicon.1.pod
	pod2man --release='${PACKAGE_VERSION}' --center='${PACKAGE_NAME}' \
		--section=1 $< > $@

volumeicon.5: volumeicon.5.pod
	pod2man --release='${PACKAGE_VERSION}' --center='${PACKAGE_NAME}' \
		--section=5 $< > $@

CLEANFILES = volumeicon.1 volumeicon.5

