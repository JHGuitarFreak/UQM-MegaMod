#!/usr/bin/perl

use strict;
use warnings;

my @frameworks = ( 'Ogg', 'Vorbis', 'libpng', 'SDL2' );
my $target = 'The Ur-Quan Masters';

create_app_bundle_skeleton($target);
copy_with_version("src/res/darwin/Info.plist", "$target.app/Contents/Info.plist");
copy_file("src/res/darwin/PkgInfo", "$target.app/Contents");
copy_file("src/res/darwin/The Ur-Quan Masters.icns", "$target.app/Contents/Resources");
copy_file("content/version", "$target.app/Contents/Resources/content");
opendir my $packagedir, "dist-packages/" or die "Could not open dist-packages directory";
while (readdir $packagedir) {
    if (/\.uqm$/) {
        copy_file("dist-packages/$_", "$target.app/Contents/Resources/content/packages");
    }
}
copy_file("uqm", "$target.app/Contents/MacOS/$target");
copy_frameworks($target, @frameworks);
relink_frameworks($target, @frameworks);


sub create_app_bundle_skeleton {
    my $target = shift;
    mkdir("$target.app", 0755);
    mkdir("$target.app/Contents", 0755);
    mkdir("$target.app/Contents/Frameworks", 0755);
    mkdir("$target.app/Contents/MacOS", 0755);
    mkdir("$target.app/Contents/Resources", 0755);
    mkdir("$target.app/Contents/Resources/content", 0755);
    mkdir("$target.app/Contents/Resources/content/addons", 0755);
    mkdir("$target.app/Contents/Resources/content/packages", 0755);
}


sub copy_frameworks {
    my $target = shift;
    foreach my $fw (@_) {
        my $src = "/Library/Frameworks/$fw.framework";
        my $dest = "$target.app/Contents/Frameworks/$fw.framework";
        system("ditto \"$src\" \"$dest\"");
    }
}


sub relink_frameworks {
    my $target = shift;
    my @frameworks = @_;
    my $execfile = "$target.app/Contents/MacOS/$target";

    foreach my $fw (@frameworks) {
        my $src = "$target.app/Contents/Frameworks/$fw.framework/$fw";
        my $oldfwid = `otool -L '$src' | head -n 2 | tail -n 1`;
        $oldfwid =~ s/^\s+//;
        $oldfwid =~ s/\s.*$//g;
        my $newfwid = $oldfwid;
        $newfwid =~ s/^\/Library/\@executable_path\/../;

        system("install_name_tool -id $newfwid \"$src\"");
        foreach my $fw2 (@frameworks) {
            next if $fw eq $fw2;
            my $src2 = "$target.app/Contents/Frameworks/$fw2.framework/$fw2";
            system("install_name_tool -change $oldfwid $newfwid \"$src2\"");
        }
        system("install_name_tool -change $oldfwid $newfwid \"$execfile\"");
    }
}


sub get_uqm_version {
    open(my $versionheader, "<", "src/uqmversion.h") or die "Could not find version file";
    my $major_version = 0;
    my $minor_version = 0;
    my $patch_version = 0;
    while (<$versionheader>) {
        if (/^\s*#define\s+UQM_MAJOR_VERSION\s+(\d+)/) {
            $major_version = $1;
        }
        if (/^\s*#define\s+UQM_MINOR_VERSION\s+(\d+)/) {
            $minor_version = $1;
        }
        if (/^\s*#define\s+UQM_PATCH_VERSION\s+(\d+)/) {
            $patch_version = $1;
        }
    }
    close($versionheader) or warn "Could not close version file";
    return "${major_version}.${minor_version}.${patch_version}";
}


sub copy_file {
    my $src = shift;
    my $dest = shift;
    system("cp \"$src\" \"$dest\"");
}

sub copy_with_version {
    my ($src, $dest) = @_;
    my $uqmversion = get_uqm_version();
    open (SRCFILE, "<", $src) or die "Could not open source file '$src'";
    open (DESTFILE, ">", $dest) or die "Could not open destination file '$dest'";
    while (<SRCFILE>) {
        s/\@\@VERSION\@\@/$uqmversion/g;
        print DESTFILE;
    }
    close(DESTFILE) or warn "Could not close destination file";
    close(SRCFILE) or warn "Could not close source file";
}
