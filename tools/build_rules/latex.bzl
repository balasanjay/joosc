def latex_pdf_impl(ctx):
  # Strip off the ".tex".
  input = ctx.file.src.path[:-4]
  output = ctx.outputs.output_pdf.path
  output_dir = output[:output.rfind('/')]

  pdflatex_cmds = []
  pdflatex_cmd = "pdflatex -output-directory=%s %s.tex > %s/log.out" % (output_dir, input, output_dir)
  for i in range(ctx.attr.reps):
    pdflatex_cmds += [pdflatex_cmd]

  command = (
      "mkdir -p %s && " % output_dir +
      " && ".join(pdflatex_cmds)
  )

  ctx.action(
      inputs = [ctx.file.src],
      outputs = [ctx.outputs.output_pdf],
      command = command,
      env = {
        "PATH": "/usr/bin:/bin",
      },
  )

latex_pdf = rule(
    latex_pdf_impl,
    attrs = {
        "src": attr.label(
            allow_files = FileType([".tex"]),
            mandatory = True,
            single_file = True,
        ),
        "reps": attr.int(
            default = 1,
        ),
    },
    outputs = {
        "output_pdf": "%{name}.pdf",
    },
)
