fit.over.all.graphs <- function (Vars, Df.fit, Df.predict) {
  response.var <- if (Vars$has.cycles) "Cycles" else "Time"
  response.true <- sprintf ("%s.true", response.var)
  Predictors.all <- NULL
  Data.predicted <- NULL
  Predictions <- NULL

  Fits <- list ()
  for (arch in unique (Df.fit[["Architecture"]])) {
    for (alg in unique (Df.fit[["Algorithm"]])) {
      for (code in unique (Df.fit[["Implementation"]])) {
        cat (sprintf ("==> %s for %s on %s ...\n", code, alg, arch))
    
        # Choose subset of data to fit
        Data.fit <- subset (Df.fit
                            , Architecture == arch &
                              Algorithm == alg &
                              Implementation == code)

        # Determine predictors
        vars.key <- get.file.suffix (arch, alg, code)
        vars.file <- sprintf ("figs2/explore-corr-vars--%s.txt", vars.key)
        cat (sprintf ("    Loading predictors from: %s ...\n", vars.file))
        if (!file.exists (vars.file)) {
          stop (sprintf ("\n*** Missing model file: '%s' ***\n", vars.file))
        }
        Predictors <- as.vector (unlist (read.table (vars.file)))

        # Fit! Use nonnegative least squares without a constant term
        Fit <- lm.by.colnames (Data.fit, response.var, Predictors
                               , constant.term=CONST.TERM, nonneg=TRUE)

        cat (sprintf ("\n=== Fitted model for: %s code for %s on %s ===\n", code, alg, arch))
        print (summary (Fit))

        # Use model to predict totals
        Data.predict <- subset (Df.predict, Architecture == arch &
                                            Algorithm == alg &
                                            Implementation == code)
        Prediction <- predict.df.lm (Fit, Data.predict, response.var)
        Prediction[, response.true] <- Data.predict[, response.var]
        Prediction <- cbind (Data.predict[, Vars$Index], Prediction)
      
        cat (sprintf ("\n=== Sample predictions ===\n"))
        print (head (Prediction))

        Predictions <- rbind.fill (Predictions, Prediction)
        Data.predicted <- rbind.fill (Data.predicted, Data.predict)
        Predictors.all <- unique (c (Predictors.all, Predictors))
      } # for each code
    } # for each alg
  } # for each arch

  Fits$Data.predicted <- Data.predicted
  Fits$Predictions <- Predictions
  Fits$Predictors.all <- Predictors.all
  Fits$response.var <- response.var
  Fits$response.true <- response.true
  return (Fits)
}

# eof

