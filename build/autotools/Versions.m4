AC_DEFUN([bson_parse_version], [
    m4_define([ver_split], m4_split(m4_translit($2, [-], [.]), [\.]))
    m4_define([$1_major_version], m4_argn(1, ver_split))
    m4_define([$1_minor_version], m4_argn(2, ver_split))
    m4_define([$1_micro_version], m4_argn(3, ver_split))
    m4_define([$1_prerelease_version], m4_argn(4, ver_split))

    # Set bson_version / bson_released_version to "x.y.z" from components.
    m4_define(
        [$1_version],
        m4_ifset(
            [$1_prerelease_version],
            [$1_major_version.$1_minor_version.$1_micro_version-$1_prerelease_version],
            [$1_major_version.$1_minor_version.$1_micro_version]))

    # E.g., if prefix is "bson", substitute BSON_MAJOR_VERSION.
    m4_define([prefix_upper], [translit([$1],[a-z],[A-Z])])
    AC_SUBST(m4_join([], prefix_upper, [_MAJOR_VERSION]), $1_major_version)
    AC_SUBST(m4_join([], prefix_upper, [_MINOR_VERSION]), $1_minor_version)
    AC_SUBST(m4_join([], prefix_upper, [_MICRO_VERSION]), $1_micro_version)
    AC_SUBST(m4_join([], prefix_upper, [_PRERELEASE_VERSION]), $1_prerelease_version)

    # Substitute the joined version string, e.g. set BSON_VERSION to $bson_version.
    AC_SUBST(m4_join([], prefix_upper, [_VERSION]), $1_version)
])

# Parse version, perhaps like "x.y.z-dev", from VERSION_CURRENT file.
bson_parse_version(bson, m4_esyscmd_s(cat VERSION_CURRENT))

# Parse most recent stable release, like "x.y.z", from VERSION_RELEASED file.
bson_parse_version(bson_released, m4_esyscmd_s(cat VERSION_RELEASED))

AC_MSG_NOTICE([Current version (from VERSION_CURRENT file): $BSON_VERSION])

m4_ifset([bson_released_prerelease_version],
         [AC_ERROR([RELEASED_VERSION file has prerelease version $BSON_RELEASED_VERSION])])

if test "x$bson_version" != "x$bson_released_version"; then
    AC_MSG_NOTICE([Most recent release (from VERSION_RELEASED file): $BSON_RELEASED_VERSION])
    m4_ifset([bson_prerelease_version], [], [
        AC_ERROR([Current version must be a prerelease (with "-dev", "-beta", etc.) or equal to previous release])
    ])
fi

# bump up by 1 for every micro release with no API changes, otherwise
# set to 0. after release, bump up by 1
m4_define([bson_released_interface_age], bson_released_micro_version)
m4_define([bson_released_binary_age],
          [m4_eval(100 * bson_released_minor_version +
                   bson_released_micro_version)])

AC_SUBST([BSON_RELEASED_INTERFACE_AGE], [bson_released_interface_age])
AC_MSG_NOTICE([libbson interface age $BSON_RELEASED_INTERFACE_AGE])

m4_define([lt_current],
          [m4_eval(100 * bson_released_minor_version +
                   bson_released_micro_version -
                   bson_released_interface_age)])

m4_define([lt_revision], [bson_released_interface_age])

m4_define([lt_age], [m4_eval(bson_released_binary_age -
                     bson_released_interface_age)])
