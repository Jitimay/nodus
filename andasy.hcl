# andasy.hcl app configuration file generated for nodus-ai on Friday, 10-Apr-26 17:31:03 CAT
#
# See https://github.com/quarksgroup/andasy-cli for information about how to use this file.

app_name = "nodus-ai"

app {

  env = {}

  port = 8000

  primary_region = "kgl"

  compute {
    cpu      = 1
    memory   = 256
    cpu_kind = "shared"
  }

  process {
    name = "nodus-ai"
  }

}
