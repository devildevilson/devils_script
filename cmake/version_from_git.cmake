# Derive a SemVer version from the nearest reachable vMAJOR.MINOR.PATCH tag.
#
# The numeric tag version is suitable for project(VERSION). Development builds also get a
# descriptive version such as 1.2.1-dev.2+g59cfa69; a modified worktree adds ".dirty".
function(ds_version_from_git source_dir out_version out_core_version)
  set(_fallback "${DS_VERSION_FALLBACK}")
  if (NOT _fallback MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
    message(FATAL_ERROR
      "DS_VERSION_FALLBACK must have the form MAJOR.MINOR.PATCH, got '${_fallback}'")
  endif()

  set(_version "${_fallback}")
  set(_core_version "${_fallback}")
  find_program(_git_executable NAMES git NO_CACHE)

  if (_git_executable)
    execute_process(
      COMMAND "${_git_executable}" -C "${source_dir}" rev-parse --show-toplevel
      RESULT_VARIABLE _top_level_result
      OUTPUT_VARIABLE _top_level
      ERROR_QUIET
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    file(REAL_PATH "${source_dir}" _source_real)
    if (_top_level_result EQUAL 0)
      file(REAL_PATH "${_top_level}" _top_level_real)
    endif()

    # Do not accidentally take a containing application's version when an unpacked source archive
    # lives somewhere below that application's Git root.
    if (_top_level_result EQUAL 0 AND _top_level_real STREQUAL _source_real)
      execute_process(
        COMMAND "${_git_executable}" -C "${source_dir}" describe
                --tags --long --dirty --match "v[0-9]*"
        RESULT_VARIABLE _describe_result
        OUTPUT_VARIABLE _description
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
    else()
      set(_describe_result 1)
    endif()

    if (_describe_result EQUAL 0 AND
        _description MATCHES
          "^v([0-9]+)\\.([0-9]+)\\.([0-9]+)-([0-9]+)-g([0-9a-f]+)(-dirty)?$")
      set(_core_version "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
      set(_distance "${CMAKE_MATCH_4}")
      set(_commit "${CMAKE_MATCH_5}")
      set(_dirty "${CMAKE_MATCH_6}")
      set(_version "${_core_version}")

      if (NOT _distance STREQUAL "0")
        set(_version "${_version}-dev.${_distance}+g${_commit}")
      endif()
      if (_dirty)
        if (_distance STREQUAL "0")
          string(APPEND _version "+dirty")
        else()
          string(APPEND _version ".dirty")
        endif()
      endif()
    elseif (_describe_result EQUAL 0)
      message(WARNING
        "Ignoring Git description '${_description}': expected a vMAJOR.MINOR.PATCH tag")
    else()
      # A repository may not have any tags yet. Retain the fallback numeric version, but expose
      # the commit in the descriptive version when possible.
      if (_top_level_result EQUAL 0 AND _top_level_real STREQUAL _source_real)
        execute_process(
          COMMAND "${_git_executable}" -C "${source_dir}" rev-parse --short HEAD
          RESULT_VARIABLE _revision_result
          OUTPUT_VARIABLE _revision
          ERROR_QUIET
          OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        if (_revision_result EQUAL 0)
          set(_version "${_fallback}-dev+g${_revision}")
        endif()
      endif()
    endif()
  endif()

  set(${out_version} "${_version}" PARENT_SCOPE)
  set(${out_core_version} "${_core_version}" PARENT_SCOPE)
endfunction()
